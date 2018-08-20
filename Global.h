#pragma once
#include <QtCore/QSemaphore>

#define  Que_Max_LoadedBuffer 200

QSemaphore SemphoreFree(Que_Max_LoadedBuffer);
QSemaphore SemphoreUsed;