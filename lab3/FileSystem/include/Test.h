#include <iostream>
#include <string>
#include <Ext2.h>
#include <Log.h>
#include <cstring>

void test01() {
    Ext2::GetInstance().init();

    // **********************************************************
    // Test

    //  /
    // Ext2::GetInstance().ext2_cd(".");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // //  /test/
    // Ext2::GetInstance().ext2_mkdir("test");
    // Ext2::GetInstance().ext2_cd("test");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // //  /test/a/
    // Ext2::GetInstance().ext2_mkdir("a");
    // Ext2::GetInstance().ext2_cd("a");
    // Ext2::GetInstance().ext2_touch("a.txt");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // //  /test/
    // Ext2::GetInstance().ext2_cd("..");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_ls();
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_rmdir("a");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_ls();
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // //  /
    // Ext2::GetInstance().ext2_cd("..");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_touch("a.txt");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_ls();
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_open("a.txt");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_l_open_file();

    // Ext2::GetInstance().ext2_write("a.txt");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_read("a.txt");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_close("a.txt");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_ll();
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().showDiskInfo();

    // Ext2::GetInstance().ext2_rm("a.txt");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_ll();
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_rmdir("test");
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().ext2_ll();
    // INFO(Ext2::GetInstance().current_path);
    // INFO("\n");

    // Ext2::GetInstance().showDiskInfo();

    // **********************************************************
}

void test02() {
    Ext2::GetInstance().init();
}
