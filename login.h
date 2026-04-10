#ifndef LOGIN_H
#define LOGIN_H
#include <stdint.h>
#include "fs.h"
#include "vga.h"
#include "keyboard.h"
#include "timer.h"
#include "libc.h"
#include "fs/ata/ata.h"
#include "fs/fat/ff.h"
#include "fs/fat/diskio.h"
#include "input.h"
#define MAX_USERS 10

typedef struct {
    char username[16];
    uint32_t passhash;
    int is_sudo;
} user_t;
user_t users[MAX_USERS];
int user_count = 0;

//-------Password-------
uint32_t hash(const char *str) {
    uint32_t hash = 0x811C9DC5;
    while (*str) {
        hash ^= (uint8_t)(*str++);
        hash *= 0x01000193;
    }
    return hash;
}

int compare(char* input, uint32_t passhash) {
    return (hash(input) == passhash);
}

//Help function
void u32_to_str(uint32_t val, char* buf) {
    int i = 10;
    buf[10] = 0;
    if(val == 0) { buf[0]='0'; buf[1]=0; return; }
    while(val && i>0){
        i--;
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
    int j = 0;
    while(buf[i]) buf[j++] = buf[i++];
    buf[j] = 0;
}

//-------Users-------
//-----Make-----
void make_user() {
    vga_print("-----Create User-----\n");
    while (1) {
        char user[128];
        char pass[128];
        char line[256];
        vga_print("Enter username: ");
        input(user, sizeof(user));
        int exists = 0;
        for(int i = 0; i < user_count; i++) {
            if(strcmp(users[i].username, user) == 0) {
                vga_print("Username already exists!\n");
                exists = 1;
            }
        }
        if (exists == 1) {continue;}
        vga_print("Enter password: ");
        input(pass, sizeof(pass));
        uint32_t passhash = hash(pass);
        int i = 0;
        while(user[i]) { line[i] = user[i]; i++; }
        line[i++] = ':';
        char hash_str[12];
        u32_to_str(passhash, hash_str);
        int j = 0;
        while(hash_str[j]) { line[i++] = hash_str[j++]; }
        line[i++] = '\n';
        line[i] = 0;
        //Make in disk
        fs_append("users.txt", (uint8_t*)line, i);
        //Make in RAM
        for(i = 0; i < 15 && user[i]; i++) {
            users[0].username[i] = user[i];
        }
        users[user_count].username[i] = 0;
        users[user_count].passhash = passhash;
        vga_print("User created!\n");
        vga_print("Rebooting");
        delay_ms(500);
        reboot();
        break;
    }
}
//-----Delete-----
void delete_user() {
    vga_print("-----Delete User-----\n");
    char username[128];
    vga_print("Enter username to delete: ");
    input(username, sizeof(username));
    int found = -1;
    for(int i = 0; i < user_count; i++) {
        if(strcmp(users[i].username, username) == 0) {
            found = i;
            break;
        }
    }
    if(found == -1) {
        vga_print("User not found!\n");
        return;
    }
    for(int i = found; i < user_count - 1; i++) {
        users[i] = users[i + 1];
    }
    user_count--;
    fs_write("users.txt", (uint8_t*)"", 0);
    for(int i = 0; i < user_count; i++) {
        char line[256];
        int pos = 0;
        int j = 0;
        while(users[i].username[j] && j < 15) {
            line[pos++] = users[i].username[j++];
        }
        line[pos++] = ':';
        char hash_str[12];
        u32_to_str(users[i].passhash, hash_str);
        j = 0;
        while(hash_str[j]) line[pos++] = hash_str[j++];
        line[pos++] = '\n';
        line[pos] = 0;
        fs_append("users.txt", (uint8_t*)line, pos);
    }

    vga_print("User deleted!\n");
}

//-----Load-----
void load_users() {
    uint8_t buf[1024];
    int n = fs_read("users.txt", buf, sizeof(buf)-1);
    if(n >= 0) {
        buf[n] = 0;
        char *line = (char*)buf;
        while(line && *line && user_count < MAX_USERS) {
            char *end = line;
            while(*end && *end != '\n') end++;
            if(*end) *end++ = 0;
            char *sep = line;
            while(*sep && *sep != ':') sep++;
            if(*sep == 0) { line = end; continue; }
            *sep = 0;
            char *username = line;
            char *password = sep+1;
            for(int i=0;i<16 && username[i];i++)
                users[user_count].username[i] = username[i];
            uint32_t hashval = 0;
            for(int k=0; password[k] >= '0' && password[k] <= '9'; k++) {
                hashval = hashval*10 + (password[k]-'0');
            }
            users[user_count].passhash = hashval;
            user_count++;
            line = end;
        }
    } else {
        vga_print("seems like you're new\n");
        make_user();
    }
}
//Help function
void clearchar(char *string, int len) {
    for(int i = 0; i < len; i++) string[i] = 0;
}

int user_exists(char* user) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, user) == 0) {
            return 1;
        }
    }
    return 0;
}

//-------Login-------
char* login() {
    vga_clear();
    vga_print("-----Login-----\n");
    char username[128];
    char password[128];
    int user = 0;
    int tries = 0;
    while (1) {
        if (tries == 3) {
            return "0";
        }
        clearchar(username, sizeof(username));
        clearchar(password, sizeof(password));
        vga_print("Username: ");
        input(username, sizeof(username));
        vga_print("Password: ");
        input(password, sizeof(password));
        int founduser = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, username) == 0) {founduser = 1; user = i;}
        }
        if (founduser == 0) {
            vga_print("User not found!\n");
            delay_ms(500);
            tries++;
            continue;
        } else {
            if (strcmp(users[user].username, username) == 0 && users[user].passhash == hash(password)) {
                vga_print("Welcome ");
                vga_print(username);
                vga_print("\n");
                return users[user].username;
            } else if(strcmp(users[user].username, username) == 0 && users[user].passhash != hash(password)) {
                vga_print("Wrong password!\n");
                delay_ms(500);
                tries++;
                continue;
            }
        }
    }
}

//-------Authenticate-------
int authenticate() {
    char username[16];
    char password[128];
    int user = 0;
    vga_print("Username: ");
    input(username, sizeof(username));
    vga_print("Password: ");
    input(password, sizeof(username));
    int founduser = 0;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {founduser = 1; user = i;}
    }
    if (founduser == 0) {
        vga_print("User not found!\n");
        return 0;
    } else {
        if (strcmp(users[user].username, username) == 0 && users[user].passhash == hash(password)) {
            vga_print("Authentication successful\n");
            return 1;
        } else if(strcmp(users[user].username, username) == 0 && users[user].passhash != hash(password)) {
            vga_print("Authentication failed!\n");
            return 0;
        }
    }
    vga_print("RETURNING AUTH\n");
    return 0;
}

#endif