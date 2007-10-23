#include <stdint.h>

typedef struct
{
  uint8_t cam_id;
  uint8_t sendnumber;
  uint64_t framenumber;
  uint16_t width;
  uint16_t height;
  uint16_t roi_x;
  uint16_t roi_y;
  double timestamp;

  uint64_t start_offset; // pointer to SHM start
  uint16_t stride;       // row stride
} shmwrap_msg_ready_t;

#define shmwrap_ftok_path "/tmp/shmwrap-sender"
#define shmwrap_shm_name 'A'

// UDP
#define shmwrap_frame_ready_port 20493

// TCP
#define shmwrap_control_port 20494

// 10 MB
#define shm_size 6*1024*1024
