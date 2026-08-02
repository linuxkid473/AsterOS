/* This is a generated file, don't edit */

#define NUM_APPLETS 11
#define KNOWN_APPNAME_OFFSETS 0

const char applet_names[] ALIGN1 = ""
"ash" "\0"
"cat" "\0"
"clear" "\0"
"echo" "\0"
"kill" "\0"
"ls" "\0"
"mkdir" "\0"
"pwd" "\0"
"rm" "\0"
"sh" "\0"
"uname" "\0"
;

#define APPLET_NO_ash 0
#define APPLET_NO_cat 1
#define APPLET_NO_clear 2
#define APPLET_NO_echo 3
#define APPLET_NO_kill 4
#define APPLET_NO_ls 5
#define APPLET_NO_mkdir 6
#define APPLET_NO_pwd 7
#define APPLET_NO_rm 8
#define APPLET_NO_sh 9
#define APPLET_NO_uname 10

#ifndef SKIP_applet_main
int (*const applet_main[])(int argc, char **argv) = {
ash_main,
cat_main,
clear_main,
echo_main,
kill_main,
ls_main,
mkdir_main,
pwd_main,
rm_main,
ash_main,
uname_main,
};
#endif

const uint8_t applet_flags[] ALIGN1 = {
0xf0,
0xfb,
0x32,
};

