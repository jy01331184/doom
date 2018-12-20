package com.doom;

public class Entity {

    SubEntry[] subEntries = new SubEntry[1024*10];

    public Entity() {
        for (int i = 0; i < subEntries.length; i++) {
            subEntries[i] = new SubEntry();
        }
    }

    class SubEntry {
        long a = 1;
        long b = 2;
        long c = 3;
        long d = 4;

        long aa = 1;
        long bb = 2;
        long cc = 3;
        long dd = 4;

        long aaa = 1;
        long bbb = 2;
        long ccc = 3;
        long ddd = 4;

        long aaaaa = 1;
        long bbbbb = 2;
        long ccccc = 3;
        long ddddd = 4;
    }
}
