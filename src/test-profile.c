#include "dh-profile.c"

int
main (int argc, char **argv)
{
        DhProfile *profile;

        gnome_vfs_init ();
        
        profile = dh_profile_new ();

        return 0;
}
