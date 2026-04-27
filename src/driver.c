/* n00b5_d0_n0t_t0uch */

#include "cdecl.h"

int PRE_CDECL asm_main( int argc, char* argv[] ) POST_CDECL;
int PRE_CDECL check_for_collision( int player_x, int player_y ) POST_CDECL;
void PRE_CDECL move_player( int* new_yx, int key, int start_x, int start_y, int lines, int cols ) POST_CDECL;

int main(int argc, char *argv[])
{
  int ret_status;
  ret_status = asm_main(argc, argv);
  return ret_status;
}