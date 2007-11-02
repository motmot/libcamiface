#emacs, this is -*-Python-*- mode

cimport c_lib

cdef extern from "sys/ipc.h":
    ctypedef int key_t
    key_t ftok(char *pathname, int proj_id)

cdef extern from "sys/shm.h":
    int shmget(key_t key, c_lib.size_t size, int shmflg)
    void *shmat(int shmid, void *shmaddr, int shmflg)
    int SHM_RDONLY
