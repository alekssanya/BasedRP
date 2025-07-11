#include "g_local.h"
#include "sqlite3.h"

sqlite3* db = NULL;

void db_init(void) {
	const char* dbFileName = "BasedRP.db";

	int rc = sqlite3_open(dbFileName, &db);
	if (rc) {
		G_Printf("Не удалось открыть БД: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		db = NULL;
		return;
	}

	G_Printf("Открыта БД: %s\n", dbFileName);

	const char* usersTableSQL =
		"CREATE TABLE IF NOT EXISTS Users ("
		"AccountID INTEGER PRIMARY KEY AUTOINCREMENT,"
		"Username TEXT UNIQUE,"
		"Password TEXT,"
		"AdminLevel TEXT CHECK (AdminLevel IN ('player', 'admin', 'god')) DEFAULT 'player',"
		"SkillPoints INTEGER DEFAULT 1,"
		"ModelScale INTEGER DEFAULT 100,"
		"LoggedIn INTEGER DEFAULT 0,"
		"ClientID INTEGER"
		")";
	char* errMsg = NULL;

	rc = sqlite3_exec(db, usersTableSQL, NULL, NULL, &errMsg);
	if (rc != SQLITE_OK) {
		G_Printf("Ошибка при создании таблицы Users: %s\n", errMsg);
		sqlite3_free(errMsg);
	}
}

// Вспомогательная функция
void SendPrint(gentity_t* ent, const char* msg) {
	trap_SendServerCommand(ent - g_entities, va("print \"%s\n\"", msg));
}

void Cmd_Register_f(gentity_t* ent) {
	if (trap_Argc() < 3) {
		SendPrint(ent, "Использование: /register [логин] [пароль]");
		return;
	}

	char username[64], password[64];
	trap_Argv(1, username, sizeof(username));
	trap_Argv(2, password, sizeof(password));

	char sql[256];
	Com_sprintf(sql, sizeof(sql),
		"INSERT INTO Users (Username, Password, LoggedIn, AdminLevel, SkillPoints, ClientID) "
		"VALUES ('%s', '%s', 0, 'player', 1, %i);",
		username, password, ent->s.number);

	char* errMsg = NULL;
	int execResult = sqlite3_exec(db, sql, 0, 0, &errMsg);
	if (execResult != SQLITE_OK) {
		SendPrint(ent, "Ошибка регистрации (возможно, логин уже существует)");
		sqlite3_free(errMsg);
	}
	else {
		SendPrint(ent, "Успешная регистрация");
	}
}

void Cmd_Login_f(gentity_t* ent) {
	if (trap_Argc() < 3) {
		SendPrint(ent, "Использование: /login [логин] [пароль]");
		return;
	}

	char username[64], password[64];
	trap_Argv(1, username, sizeof(username));
	trap_Argv(2, password, sizeof(password));

	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db,
		"SELECT Password, SkillPoints, ModelScale FROM Users WHERE Username = ?;",
		-1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		SendPrint(ent, "Ошибка входа");
		return;
	}

	sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		const char* dbPass = (const char*)sqlite3_column_text(stmt, 0);
		if (strcmp(password, dbPass) == 0) {
			int skillPoints = sqlite3_column_int(stmt, 1);
			int modelScale = sqlite3_column_int(stmt, 2);

			ent->client->sess.skillPoints = skillPoints;
			ent->client->sess.modelScale = modelScale;
			ent->client->ps.iModelScale = modelScale;
			Q_strncpyz(ent->client->sess.username, username, sizeof(ent->client->sess.username));
			char sql[128];
			Com_sprintf(sql, sizeof(sql),
				"UPDATE Users SET LoggedIn = 1, ClientID = %i WHERE Username = '%s';",
				ent->s.number, username);
			sqlite3_exec(db, sql, 0, 0, NULL);

			SendPrint(ent, "Вход выполнен");
		}
		else {
			SendPrint(ent, "Неверный пароль");
		}
	}
	else {
		SendPrint(ent, "Аккаунт не найден");
	}

	sqlite3_finalize(stmt);
}

void Cmd_Logout_f(gentity_t* ent) {
	if (ent->client->sess.username[0] == '\0') {
		SendPrint(ent, "Вы не вошли в аккаунт");
		return;
	}

	char sql[256];
	Com_sprintf(sql, sizeof(sql),
		"UPDATE Users SET LoggedIn = 0 WHERE Username = '%s';",
		ent->client->sess.username);

	sqlite3_exec(db, sql, 0, 0, NULL);

	// Очистить имя пользователя
	ent->client->sess.username[0] = '\0';

	SendPrint(ent, "Вы вышли из аккаунта");
}

void Cmd_AddSkillPoint_f(gentity_t* ent) {
	if (!HasAdminRights(ent)) {
		SendPrint(ent, "Недостаточно прав для выполнения команды");
		return;
	}

	if (trap_Argc() < 3) {
		SendPrint(ent, "Использование: /addskillpoint [логин] [кол-во]");
		return;
	}

	char username[64], amountStr[16];
	trap_Argv(1, username, sizeof(username));
	trap_Argv(2, amountStr, sizeof(amountStr));

	int amount = atoi(amountStr);
	if (amount <= 0) {
		SendPrint(ent, "Некорректное количество очков");
		return;
	}

	// Проверка текущего значения из БД
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db,
		"SELECT SkillPoints, ClientID FROM Users WHERE Username = ?;",
		-1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		SendPrint(ent, "Ошибка при проверке очков");
		return;
	}

	sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		int currentPoints = sqlite3_column_int(stmt, 0);
		int clientId = sqlite3_column_int(stmt, 1);

		if (currentPoints + amount > 600) {
			SendPrint(ent, "Нельзя превышать 600 очков навыков");
			sqlite3_finalize(stmt);
			return;
		}

		sqlite3_finalize(stmt);

		// Обновление в БД
		char sql[256];
		Com_sprintf(sql, sizeof(sql),
			"UPDATE Users SET SkillPoints = SkillPoints + %d WHERE Username = '%s';",
			amount, username);
		sqlite3_exec(db, sql, 0, 0, NULL);

		// Обновление у игрока, если онлайн
		if (clientId >= 0 && clientId < MAX_CLIENTS) {
			gentity_t* target = &g_entities[clientId];
			if (target && target->inuse && target->client) {
				target->client->sess.skillPoints += amount;
			}
		}

		// Сообщение
		char msg[128];
		Com_sprintf(msg, sizeof(msg), "Добавлено %d очков пользователю %s", amount, username);
		SendPrint(ent, msg);

	}
	else {
		sqlite3_finalize(stmt);
		SendPrint(ent, "Пользователь не найден");
	}
}

void Cmd_SetModelScale_f(gentity_t* ent) {
	if (!HasAdminRights(ent)) {
		SendPrint(ent, "Недостаточно прав для выполнения команды");
		return;
	}

	if (trap_Argc() < 3) {
		SendPrint(ent, "Использование: /mdscl <clientID> <scale>");
		return;
	}

	char clientIdStr[16];
	char scaleStr[16];
	int clientID, scale;

	trap_Argv(1, clientIdStr, sizeof(clientIdStr));
	trap_Argv(2, scaleStr, sizeof(scaleStr));

	clientID = atoi(clientIdStr);
	scale = atoi(scaleStr);

	if (clientID < 0 || clientID >= level.maxclients) {
		SendPrint(ent, "Неверный ClientID");
		return;
	}

	if (scale < 25 || scale > 300) {
		SendPrint(ent, "Недопустимый масштаб (25–300)");
		return;
	}

	// Обновляем базу
	char sql[256];
	Com_sprintf(sql, sizeof(sql),
		"UPDATE Users SET ModelScale = %i WHERE ClientID = %i;",
		scale, clientID);
	sqlite3_exec(db, sql, 0, 0, NULL);

	// Обновляем масштаб игроку, если он в игре
	gentity_t* target = &g_entities[clientID];
	if (target && target->inuse && target->client) {
		target->client->sess.modelScale = scale;
		target->client->ps.iModelScale = scale;
		target->s.iModelScale = scale;
	}

	SendPrint(ent, va("Масштаб игрока с ClientID %i установлен: %i", clientID, scale));
}


//не работает потому что ClientDisconnect по какой то причине не вызывается
void Clear_DB(gentity_t* ent) {
	if (1 > 2) { //всегда ложно
		char sql[128];
		Com_sprintf(sql, sizeof(sql),
			"UPDATE Users SET LoggedIn = 0, ClientID = NULL WHERE Username = '%s';",
			ent->client->sess.username);
		sqlite3_exec(db, sql, 0, 0, NULL);
	}
}

qboolean HasAdminRights(gentity_t* ent) {
	if (ent->client->sess.username[0] == '\0') {
		return qfalse; // не залогинен
	}

	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db,
		"SELECT AdminLevel FROM Users WHERE Username = ? AND LoggedIn = 1;",
		-1, &stmt, NULL);

	if (rc != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return qfalse;
	}

	sqlite3_bind_text(stmt, 1, ent->client->sess.username, -1, SQLITE_STATIC);

	qboolean result = qfalse;

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		const char* level = (const char*)sqlite3_column_text(stmt, 0);
		if (level && (strcmp(level, "admin") == 0 || strcmp(level, "god") == 0)) {
			result = qtrue;
		}
	}

	sqlite3_finalize(stmt);
	return result;
}

/*
void Cmd_CharCreate_f(gentity_t* ent) {
	if (trap_Argc() < 2) {
		SendPrint(ent, "Использование: /charcreate [имя]");
		return;
	}

	char charName[64];
	trap_Argv(1, charName, sizeof(charName));

	char sql[512];
	Com_sprintf(sql, sizeof(sql),
		"INSERT INTO Characters (Name, AccountID, ModelScale, SkillPoints) "
		"SELECT '%s', AccountID, 100, 0 FROM Users WHERE ClientID = %i AND LoggedIn = 1;",
		charName, ent->s.number);

	char* errMsg = 0;
	int rc = sqlite3_exec(db, sql, 0, 0, &errMsg);
	if (rc != SQLITE_OK) {
		SendPrint(ent, "Ошибка создания персонажа (возможно, имя занято)");
		sqlite3_free(errMsg);
	}
	else {
		SendPrint(ent, "Персонаж создан");
	}
}
void Cmd_CharSelect_f(gentity_t* ent) {
	if (trap_Argc() < 2) {
		SendPrint(ent, "Использование: /char [имя]");
		return;
	}

	char name[64];
	trap_Argv(1, name, sizeof(name));

	sqlite3_stmt* stmt;
	const char* sql =
		"SELECT CharID FROM Characters WHERE Name = ? AND AccountID IN "
		"(SELECT AccountID FROM Users WHERE ClientID = ? AND LoggedIn = 1);";

	int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (rc != SQLITE_OK) {
		SendPrint(ent, "Ошибка БД");
		return;
	}

	sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 2, ent->s.number);

	if (sqlite3_step(stmt) == SQLITE_ROW) {
		SendPrint(ent, "Персонаж выбран");
	}
	else {
		SendPrint(ent, "Персонаж не найден");
	}

	sqlite3_finalize(stmt);
}
*/
