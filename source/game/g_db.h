#pragma once
#ifndef G_DB_H
#define G_DB_H

#include "g_local.h"
#include "sqlite3.h"

// Инициализация и закрытие базы данных
void DB_Init(void);
void DB_Close(void);

// Команды игрока
void Cmd_Register_f(gentity_t* ent);
void Cmd_Login_f(gentity_t* ent);
void Cmd_CharCreate_f(gentity_t* ent);
void Cmd_CharSelect_f(gentity_t* ent);
void Cmd_Logout_f(gentity_t* ent);
void Cmd_AddSkillPoint_f(gentity_t* ent);

qboolean HasAdminRights(gentity_t* ent);
void SendPrint(gentity_t* ent, const char* msg);
void Clear_DB(gentity_t* ent);

#endif // G_DB_H
