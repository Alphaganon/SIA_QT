// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "geometryengine.h"
#include "../super_parser.h"

#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

//! [0]
GeometryEngine::GeometryEngine(Joint *root, std::vector<Joint*> jntVec, bool skin)
    : indexBuf(QOpenGLBuffer::IndexBuffer)
{
    initializeOpenGLFunctions();

    // Generate 2 VBOs
    arrayBuf.create();
    indexBuf.create();

    // Initializes cube geometry and transfers it to VBOs
    this->skin = skin;
    this->jntVec = jntVec;
    if (skin){
        initSkinGeometry(root);
    }
    else{
        initLineGeometry(root);
    }
    //initCubeGeometry();
}

GeometryEngine::~GeometryEngine()
{
    arrayBuf.destroy();
    indexBuf.destroy();
}
//! [0]

void GeometryEngine::initCubeGeometry()
{
    // For cube we would need only 8 vertices but we have to
    // duplicate vertex for each face because texture coordinate
    // is different.
    VertexData vertices[] = {
        // Vertex data for face 0
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.0f, 0.0f)},  // v0
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D(0.33f, 0.0f)}, // v1
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.0f, 0.5f)},  // v2
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.33f, 0.5f)}, // v3

        // Vertex data for face 1
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D( 0.0f, 0.5f)}, // v4
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.33f, 0.5f)}, // v5
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.0f, 1.0f)},  // v6
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.33f, 1.0f)}, // v7

        // Vertex data for face 2
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.5f)}, // v8
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(1.0f, 0.5f)},  // v9
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.66f, 1.0f)}, // v10
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(1.0f, 1.0f)},  // v11

        // Vertex data for face 3
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.0f)}, // v12
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(1.0f, 0.0f)},  // v13
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.66f, 0.5f)}, // v14
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(1.0f, 0.5f)},  // v15

        // Vertex data for face 4
        {QVector3D(-1.0f, -1.0f, -1.0f), QVector2D(0.33f, 0.0f)}, // v16
        {QVector3D( 1.0f, -1.0f, -1.0f), QVector2D(0.66f, 0.0f)}, // v17
        {QVector3D(-1.0f, -1.0f,  1.0f), QVector2D(0.33f, 0.5f)}, // v18
        {QVector3D( 1.0f, -1.0f,  1.0f), QVector2D(0.66f, 0.5f)}, // v19

        // Vertex data for face 5
        {QVector3D(-1.0f,  1.0f,  1.0f), QVector2D(0.33f, 0.5f)}, // v20
        {QVector3D( 1.0f,  1.0f,  1.0f), QVector2D(0.66f, 0.5f)}, // v21
        {QVector3D(-1.0f,  1.0f, -1.0f), QVector2D(0.33f, 1.0f)}, // v22
        {QVector3D( 1.0f,  1.0f, -1.0f), QVector2D(0.66f, 1.0f)}  // v23
    };

    // Indices for drawing cube faces using triangle strips.
    // Triangle strips can be connected by duplicating indices
    // between the strips. If connecting strips have opposite
    // vertex order then last index of the first strip and first
    // index of the second strip needs to be duplicated. If
    // connecting strips have same vertex order then only last
    // index of the first strip needs to be duplicated.
    GLushort indices[] = {
         0,  1,  2,  3,  3,     // Face 0 - triangle strip ( v0,  v1,  v2,  v3)
         4,  4,  5,  6,  7,  7, // Face 1 - triangle strip ( v4,  v5,  v6,  v7)
         8,  8,  9, 10, 11, 11, // Face 2 - triangle strip ( v8,  v9, v10, v11)
        12, 12, 13, 14, 15, 15, // Face 3 - triangle strip (v12, v13, v14, v15)
        16, 16, 17, 18, 19, 19, // Face 4 - triangle strip (v16, v17, v18, v19)
        20, 20, 21, 22, 23      // Face 5 - triangle strip (v20, v21, v22, v23)
    };

//! [1]
    // Transfer vertex data to VBO 0
    arrayBuf.bind();
    arrayBuf.allocate(vertices, 24 * sizeof(VertexData));

    // Transfer index data to VBO 1
    indexBuf.bind();
    indexBuf.allocate(indices, 34 * sizeof(GLushort));
//! [1]
}

void GeometryEngine::copyVector(std::vector<VertexData> vecToCopy, std::vector<VertexData> &copiedVec){
    for(VertexData data : vecToCopy){
        QVector3D pos(data.position.x(), data.position.y(), data.position.z());
        QVector2D tex(data.texCoord.x(), data.texCoord.y());
        copiedVec.push_back({pos, tex});
    }
}

void GeometryEngine::getPos(Joint *jnt, std::vector<VertexData> *vec){
    QMatrix4x4 transform = *(jnt->_transform);
    Joint * parent = jnt->parent;
    while (parent != nullptr){
        transform = *(parent->_transform) * transform;
        parent = parent->parent;
    }

    vec->push_back({QVector3D(transform.column(3).x(), transform.column(3).y(), transform.column(3).z()), QVector2D(0.0f, 0.0f)});

    if(!(jnt->_children.empty())){
        for(Joint *child : jnt->_children){
            getPos(child, vec);
        }
    }
}

void GeometryEngine::getSkinPos(Joint *jnt, std::vector<VertexData> &vec){
    QMatrix4x4 transform = *(jnt->_transform);
    Joint * parent = jnt->parent;
    while (parent != nullptr){
        transform = *(parent->_transform) * transform;
        parent = parent->parent;
    }

    for (int i = 0; i < skinPos.size(); i++){
        if(weightList[i][jnt->_name] != 0){
            QVector4D pos(skinPos[i].position.x(), skinPos[i].position.y(), skinPos[i].position.z(), 1);
            QVector4D res = weightList[i][jnt->_name] * (transform * (*(jnt->_restTransform) * pos));
            vec[i].position += QVector3D(res.x(), res.y(), res.z());
        }
    }

    if(!(jnt->_children.empty())){
        for(Joint *child : jnt->_children){
            getSkinPos(child, vec);
        }
    }
}

void GeometryEngine::setWeights(std::vector<VertexData> vec){
    std::vector<QVector3D> jntPos;
    for (int i = 0; i < jntVec.size(); i++){
        Joint* jnt = jntVec[i];
        Joint * parent = jnt->parent;
        QMatrix4x4 transform = *(jnt->_transform);
        while (parent != nullptr){
            transform = *(parent->_transform) * transform;
            parent = parent->parent;
        }
        jntPos.push_back(QVector3D(transform.column(3).x(), transform.column(3).y(), transform.column(3).z()));
    }
    for(VertexData vData : vec){
        QVector3D vertexPos = vData.position;
        std::string closestJoint = "";
        double closestDist = 999999999.0;
        std::unordered_map<std::string, float> weights;
        for(int i = 0; i < jntVec.size(); i++){
            Joint* jnt = jntVec[i];
            weights[jnt->_name] = 0.0;
            QVector3D jointPos = jntPos[i];
            double dist = (double) vertexPos.distanceToPoint(jointPos);
            if (dist < closestDist){
                closestDist = dist;
                closestJoint = jnt->_name;
            }
        }
        weights[closestJoint] = 1.0;
        // std::cout << "JOINT : " << closestJoint << " DIST : " << closestDist << std::endl;
        weightList.push_back(weights);
    }
}

void GeometryEngine::parseWeights(std::string fileName){
    std::ifstream inputfile(fileName.data());
    if(inputfile.good()){
        bool idsFound = false;
        std::vector<std::string> ids;
        while(!inputfile.eof()) {
            std::setlocale(LC_ALL, "en_US.UTF-8");
            std::string buf;
			std::getline(inputfile, buf);
            std::vector<std::string> tokens = multiParser(buf, " \t\r\n");
            if(!idsFound){
                for(int i = 1 ; i < tokens.size() ; i++){
                    ids.push_back(tokens[i]);
                }
                idsFound = true;
                continue;
            }
            std::unordered_map<std::string, float> weights;
            for(int i = 1 ; i < tokens.size() ; i++){
                weights[ids[i-1]] = stof(tokens[i]);
            }
            weightList.push_back(weights);
        }
    }
}

void GeometryEngine::setJointIndexes(Joint *jnt, int &vertexIndex){
    jnt->index = vertexIndex++;
    if(!(jnt->_children.empty())){
        for(Joint *child : jnt->_children){
            setJointIndexes(child, vertexIndex);
        }
    }
}

void GeometryEngine::setIndexes(Joint *jnt, std::vector<GLushort> *vec){
    if(!(jnt->_children.empty())){
        for(Joint *child : jnt->_children){
            vec->push_back(jnt->index);
            vec->push_back(child->index);
            setIndexes(child, vec);
        }
    }
}

// std::vector<double> GeometryEngine::getScales(){
//     //moyennes xyz puis ecart moyen a la moyenne (absolu) 
//     std::vector<double> max;
//     for (Joint j : jntVec){
//         for ()
//     }
// }

void GeometryEngine::initLineGeometry(Joint *root)
{
    std::vector<VertexData> vec; 
    getPos(root, &vec);
    VertexData *vertices = &vec[0];
    int lenVec = vec.size();

    int vertexIndex = 0;

    setJointIndexes(root, vertexIndex);
    std::vector<GLushort> indVec;
    setIndexes(root, &indVec);
    int lenIdx = indVec.size();

    GLushort *indices = &indVec[0];

    // for(VertexData v : vec){
        //std::cout << v.position.x() << " " << v.position.y() << " " << v.position.z() << std::endl;
    // }

    // Transfer vertex data to VBO 0
    arrayBuf.bind();
    arrayBuf.allocate(vertices, lenVec * sizeof(VertexData));

    // Transfer index data to VBO 1
    indexBuf.bind();
    indexBuf.allocate(indices, lenIdx * sizeof(GLushort));

    lenPts = lenVec;
    lenIndexes = lenIdx;
}

void GeometryEngine::initSkinGeometry(Joint *root)
{
    std::string filename = "../skin.off";
    std::pair<std::vector<VertexData>, std::vector<unsigned short>> p = parseVertex(filename);
    int lenVec = p.first.size();
    int lenIdx = p.second.size();

    skinPos = p.first;
    std::vector<VertexData> newPos;
    for(int i = 0 ; i < skinPos.size() ; i++){
        VertexData newVert = {skinPos[i].position + QVector3D(root->_offX, root->_offY, root->_offZ), QVector2D(0.0f, 0.0f)};
        newPos.push_back(newVert);
    }
    skinPosCopy = newPos;

    parseWeights("../weights.txt");
    // setWeights(skinPosCopy);

    for(Joint * jnt : jntVec){
        //transform back to 0 0 0
        *(jnt->_restTransform) *= root->_transform->inverted();
        // invert
        *(jnt->_restTransform) = jnt->_restTransform->inverted();

    }
    
    VertexData *vertices = &(p.first[0]);
    GLushort *indices = &(p.second[0]);

    // Transfer vertex data to VBO 0
    arrayBuf.bind();
    arrayBuf.allocate(vertices, lenVec * sizeof(VertexData));

    // Transfer index data to VBO 1
    indexBuf.bind();
    indexBuf.allocate(indices, lenIdx * sizeof(GLushort));

    lenPts = lenVec;
    lenIndexes = lenIdx;
}

void GeometryEngine::updatePos(Joint *root){
    std::vector<VertexData> vec;
    getPos(root, &vec);
    VertexData *vertices = &vec[0];
    int lenVec = vec.size();
    // Transfer vertex data to VBO 0
    arrayBuf.destroy();
    arrayBuf.create(); 
    arrayBuf.bind();
    arrayBuf.allocate(vertices, lenVec * sizeof(VertexData));
}

void GeometryEngine::updateSkinPos(Joint *root){
    for (int i = 0; i< skinPosCopy.size(); i++){
        skinPosCopy[i].position = QVector3D(0, 0, 0);
    }
    getSkinPos(root, skinPosCopy);
    // std::cout << skinPos[0].position.x() << std::endl;
    VertexData *vertices = &skinPosCopy[0];
    int lenVec = skinPosCopy.size();
    // Transfer vertex data to VBO 0
    arrayBuf.destroy();
    arrayBuf.create(); 
    arrayBuf.bind();
    arrayBuf.allocate(vertices, lenVec * sizeof(VertexData));
}

//! [2]
void GeometryEngine::drawCubeGeometry(QOpenGLShaderProgram *program)
{
    // Tell OpenGL which VBOs to use
    arrayBuf.bind();
    indexBuf.bind();

    // Offset for position
    quintptr offset = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("a_position");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    int texcoordLocation = program->attributeLocation("a_texcoord");
    program->enableAttributeArray(texcoordLocation);
    program->setAttributeBuffer(texcoordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));

    // Draw cube geometry using indices from VBO 1
    glDrawElements(GL_TRIANGLE_STRIP, 34, GL_UNSIGNED_SHORT, nullptr);
}
//! [2]

void GeometryEngine::drawLineGeometry(QOpenGLShaderProgram *program)
{
    // Tell OpenGL which VBOs to use
    arrayBuf.bind();
    indexBuf.bind();

    // Offset for position
    quintptr offset = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("a_position");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    int texcoordLocation = program->attributeLocation("a_texcoord");
    program->enableAttributeArray(texcoordLocation);
    program->setAttributeBuffer(texcoordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));

    // Draw cube geometry using indices from VBO 1
    glDrawElements(GL_LINES, lenIndexes, GL_UNSIGNED_SHORT, nullptr);
}

void GeometryEngine::drawSkinGeometry(QOpenGLShaderProgram *program){
    // Tell OpenGL which VBOs to use
    arrayBuf.bind();
    indexBuf.bind();

    // Offset for position
    quintptr offset = 0;

    // Tell OpenGL programmable pipeline how to locate vertex position data
    int vertexLocation = program->attributeLocation("a_position");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    // Offset for texture coordinate
    offset += sizeof(QVector3D);

    // Tell OpenGL programmable pipeline how to locate vertex texture coordinate data
    int texcoordLocation = program->attributeLocation("a_texcoord");
    program->enableAttributeArray(texcoordLocation);
    program->setAttributeBuffer(texcoordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));

    // Draw cube geometry using indices from VBO 1
    glDrawElements(GL_TRIANGLES, lenIndexes, GL_UNSIGNED_SHORT, nullptr);
}