#include <Python.h>

int main() {
    // Python 모듈의 경로를 출력합니다
    Py_Initialize();
    PyObject* sysPath = PySys_GetObject("path");
    PyObject* pathList = PySequence_List(sysPath);
    Py_ssize_t pathLength = PySequence_Length(pathList);

    for (Py_ssize_t i = 0; i < pathLength; ++i) {
        PyObject* pathItem = PySequence_GetItem(pathList, i);
        const char* path = PyUnicode_AsUTF8(pathItem);
        printf("%s\n", path);
        Py_DECREF(pathItem);
    }

    Py_DECREF(pathList);
    Py_DECREF(sysPath);
    Py_Finalize();

    return 0;
}


/*
import sys
print(sys.path)

*/