# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qstring.pro.

#####################################################################
## tst_qstring Test:
#####################################################################

qt_internal_add_test(tst_qstring
    SOURCES
        tst_qstring.cpp
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qstring CONDITION NOT QT_FEATURE_doubleconversion AND NOT QT_FEATURE_system_doubleconversion
    DEFINES
        QT_NO_DOUBLECONVERSION
)

qt_internal_extend_target(tst_qstring CONDITION APPLE
    SOURCES
        tst_qstring_mac.mm
    LIBRARIES
        ${FWFoundation}
)
