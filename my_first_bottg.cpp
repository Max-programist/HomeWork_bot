const long long ADMIN_ID = ;

#include <stdio.h>
#include <tgbot/tgbot.h>
#include <sqlite3.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cstdlib>
sqlite3* db;





int init_database() {
    int rc = sqlite3_open("test.db", &db);
    if (rc) {
        std::cerr << "Ошибка: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    } 
    else {
        std::cout << "База открыта" << std::endl;
        return 0;
    }
    
    
}


void processHomeworkLine(const std::string& line) {
    
    size_t firstSpace = line.find(' ');
    if (firstSpace == std::string::npos) return;

    
    std::string dayCode = line.substr(0, firstSpace);
    std::string rest = line.substr(firstSpace + 1);

    
    size_t colonPos = rest.find(':');
    if (colonPos == std::string::npos) return;

    
    std::string subject = rest.substr(0, colonPos);
    std::string task = rest.substr(colonPos + 1);

    
    if (!task.empty() && task[0] == ' ') task = task.substr(1);

    
    std::string dayFull;
    switch (dayCode[0]) {
    case 'm': if (dayCode == "mn") dayFull = "monday"; else return; break;
    case 't': if (dayCode == "tu") dayFull = "tuesday"; else if (dayCode == "thur") dayFull = "thursday"; else return; break;
    case 'w': if (dayCode == "we") dayFull = "wednesday"; else return; break;
    case 'f': if (dayCode == "fr") dayFull = "friday"; else return; break;
    default: return;
    }

    
    sqlite3_stmt* stmt;
    const char* sql = "INSERT OR REPLACE INTO homework (day_of_week, subject, task) VALUES (?, ?, ?);";
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, dayFull.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, subject.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, task.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}


int main() {
   

    if (init_database() != 0) {
        std::cerr << "Не удалось инициализировать базу данных. Бот не запущен." << std::endl;
        return 1;  
    }
        
    const char* sqlCreate = "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER UNIQUE,"
        "username TEXT,"
        "is_banned INTEGER DEFAULT 0);";
    sqlite3_exec(db, sqlCreate, 0, 0, 0);



    const char* sqlHomework = "CREATE TABLE IF NOT EXISTS homework ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "day_of_week TEXT,"
        "subject TEXT,"
        "task TEXT);";
    sqlite3_exec(db, sqlHomework, 0, 0, 0);

    





    TgBot::Bot bot("");
    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        int userId = message->chat->id;
        std::string username = message->chat->username;

        // Сохраняем пользователя
        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO users (user_id, username) VALUES (?, ?);";
        sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, userId);
        sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // Получаем текущий день
        time_t now = time(nullptr);
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        int wday = timeinfo.tm_wday;

        std::string dayFull;
        switch (wday) {
        case 1: dayFull = "понедельник"; break;
        case 2: dayFull = "вторник"; break;
        case 3: dayFull = "среду"; break;
        case 4: dayFull = "четверг"; break;
        case 5: dayFull = "пятницу"; break;
        default: return;  // выходные — ничего не отправляем
        }

        // Запрашиваем домашку
        sqlite3_stmt* stmt2;
        const char* sql2 = "SELECT subject, task FROM homework WHERE day_of_week = ?;";
        sqlite3_prepare_v2(db, sql2, -1, &stmt2, 0);
        sqlite3_bind_text(stmt2, 1, dayFull.c_str(), -1, SQLITE_STATIC);

        std::string answer = "📚 Домашка на " + dayFull + ":\n";
        int count = 0;
        while (sqlite3_step(stmt2) == SQLITE_ROW) {
            count++;
            const char* subject = reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 0));
            const char* task = reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 1));
            answer += "• " + std::string(subject) + ": " + std::string(task) + "\n";
        }
        sqlite3_finalize(stmt2);
        });
    








    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        printf("User wrote %s\n", message->text.c_str());
        if (StringTools::startsWith(message->text, "/start")) {
            return;
          }
        });


    bot.getEvents().onCommand("users", [&bot](TgBot::Message::Ptr message) {
        if (message->chat->id != ADMIN_ID) {
            bot.getApi().sendMessage(message->chat->id, "Нет прав");
            return;
        }

        sqlite3_stmt* stmt;
        const char* sql = "SELECT user_id, username FROM users;";

        sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        std::string result = "Список пользователей:\n";
        int count = 0;

  
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            count++;
            int userId = sqlite3_column_int(stmt, 0);           
            const unsigned char* username = sqlite3_column_text(stmt, 1); 
            result += std::to_string(count) + ". " + std::to_string(userId) + " @" + reinterpret_cast<const char*>(username) + "\n";
        }

        if (count == 0) {
            result = "Пользователей пока нет";
        }

        sqlite3_finalize(stmt);
        bot.getApi().sendMessage(message->chat->id, result);
        });



   

    bot.getEvents().onCommand("doc", [&bot](TgBot::Message::Ptr message) {
        if (message->chat->id != ADMIN_ID) {
            bot.getApi().sendSticker(message->chat->id, "CAACAgIAAxkBAAEDiKpp77qOYsfFFU1uMJXz9hnRjTMfTQAC2i4AAokBmUi3lxj1vkv8ETsE");
            return;
        } 

        std::string fileId = message->document->fileId;
        std::string botToken = "8609775636:AAG3saJzVk8JB65e7HslvAj2PKqE_Kckn4Q";
        
        std::string command = "curl -s -o temp_hw.txt \"https://api.telegram.org/file/bot" + botToken + "/" + fileId + "\"";
        system(command.c_str());
        
        std::ifstream file("temp_hw.txt");
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) processHomeworkLine(line);
        }
        file.close();
        std::remove("temp_hw.txt");
        bot.getApi().sendMessage(message->chat->id, "✅ Расписание обновлено");
     });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    }
    catch (TgBot::TgException& e) {
        printf("error: %s\n", e.what());
    }
    return 0;
}
