#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_PARTICIPANTS 100
#define MAX_EVENTS 50

struct Participant {
    int id;
    char name[100];
    char sport[50];
    int eventID;
};

struct Event {
    int eventID;
    char eventName[100];
    char date[20];
    int maxParticipants;
    int currentParticipants;
};

void displayCSS();
void displayForm(struct Event events[], int numEvents);
void displayParticipants(struct Participant participants[], int numParticipants);
void displayEvents(struct Event events[], int numEvents);
void addParticipant(MYSQL *conn, const char *name, const char *sport, int eventID);
void addEvent(MYSQL *conn, const char *eventName, const char *date, int maxParticipants);
void removeParticipant(MYSQL *conn, int participantID);
void removeEvent(MYSQL *conn, int eventID);
void handleFormSubmission(MYSQL *conn);
void urlDecode(char *dst, const char *src);
void viewParticipants(MYSQL *conn, int event_id);


int main() {
    printf("Content-type: text/html\n\n");


    MYSQL *conn;
    const char *server = "localhost";
    const char *user = "root";
    const char *password = "";
    const char *database = "sports_event_db";

    conn = mysql_init(NULL);


    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
        fprintf(stderr, "Database connection error: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    handleFormSubmission(conn);

    struct Participant participants[MAX_PARTICIPANTS];
    struct Event events[MAX_EVENTS];
    int numParticipants = 0, numEvents = 0;
    if (mysql_query(conn, "SELECT * FROM participants")) {
        fprintf(stderr, "Query error (fetch participants): %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != NULL) {
        participants[numParticipants].id = atoi(row[0]);
        strcpy(participants[numParticipants].name, row[1]);
        strcpy(participants[numParticipants].sport, row[2]);
        participants[numParticipants].eventID = atoi(row[3]);
        numParticipants++;
    }
    mysql_free_result(res);


    if (mysql_query(conn, "SELECT * FROM events")) {
        fprintf(stderr, "Query error (fetch events): %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    res = mysql_store_result(conn);
    while ((row = mysql_fetch_row(res)) != NULL) {
        events[numEvents].eventID = atoi(row[0]);
        strcpy(events[numEvents].eventName, row[1]);
        strcpy(events[numEvents].date, row[2]);
        events[numEvents].maxParticipants = atoi(row[3]);
        events[numEvents].currentParticipants = atoi(row[4]);
        numEvents++;
    }
    mysql_free_result(res);


    displayCSS();
    displayForm(events, numEvents);
    displayParticipants(participants, numParticipants);
    displayEvents(events, numEvents);


    mysql_close(conn);
    return EXIT_SUCCESS;
}

void displayCSS() {
    printf("<style>\n");

    printf("body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 20px; background-color: #f4f7f6; color: #333; }\n");
    printf("h1 { font-size: 2.5em; color: #4CAF50; text-align: center; margin-bottom: 20px; }\n");
    printf("h2 { color: #333; margin-bottom: 15px; }\n");

    printf("form { margin-bottom: 20px; padding: 20px; background-color: #fff; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }\n");
    printf("input[type='text'], input[type='number'], select { width: 100%%; padding: 12px; margin: 8px 0; border-radius: 4px; border: 1px solid #ccc; font-size: 1em; }\n");
    printf("input[type='submit'] { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 1.1em; transition: background-color 0.3s ease; }\n");
    printf("input[type='submit']:hover { background-color: #45a049; }\n");
    printf("input[type='date'] {padding: 5px;font-size: 16px;border-radius: 5px;border: 1px solid #ccc;}");

    printf("table { width: 100%%; border-collapse: collapse; margin-top: 30px; background-color: #fff; border-radius: 8px; overflow: hidden; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }\n");
    printf("th, td { padding: 12px; text-align: left; border: 1px solid #ddd; }\n");
    printf("th { background-color: #f2f2f2; font-size: 1.1em; }\n");
    printf("tr:nth-child(even) { background-color: #f9f9f9; }\n");
    printf("tr:hover { background-color: #f1f1f1; }\n");


    printf(".remove-button { background-color: #e74c3c; color: white; padding: 8px 15px; border: none; border-radius: 4px; cursor: pointer; transition: background-color 0.3s ease; }\n");
    printf(".remove-button:hover { background-color: #c0392b; }\n");

    printf("@media (max-width: 768px) {\n");
    printf("body { padding: 10px; }\n");
    printf("h1 { font-size: 2em; }\n");
    printf("form { padding: 15px; }\n");
    printf("input[type='text'], input[type='number'], select, input[type='submit'] { font-size: 1em; }\n");
    printf("table { font-size: 0.9em; }\n");
    printf("}\n");


    printf("</style>\n");
}

void handleFormSubmission(MYSQL *conn) {
    char *request_method = getenv("REQUEST_METHOD");
    if (request_method != NULL && strcmp(request_method, "POST") == 0) {
        int content_length = atoi(getenv("CONTENT_LENGTH"));
        char data[content_length + 1];
        fread(data, content_length, 1, stdin);
        data[content_length] = '\0';

        char name[100] = {0}, sport[50] = {0};
        char eventName[100] = {0}, date[20] = {0};
        int maxParticipants = 0, id = 0, eventID = 0;

        if (strstr(data, "add_participant") != NULL) {
            sscanf(data, "participant_name=%99[^&]&participant_sport=%49[^&]&participant_event_id=%d", name, sport, &eventID);
            urlDecode(name, name);
            urlDecode(sport, sport);
            addParticipant(conn, name, sport, eventID);
        } else if (strstr(data, "add_event") != NULL) {
            sscanf(data, "event_name=%99[^&]&event_date=%19[^&]&max_participants=%d", eventName, date, &maxParticipants);
            urlDecode(eventName, eventName);
            urlDecode(date, date);
            addEvent(conn, eventName, date, maxParticipants);
        } else if (strstr(data, "remove_participant") != NULL) {
            sscanf(data, "participant_id=%d", &id);
            removeParticipant(conn, id);
        } else if (strstr(data, "remove_event") != NULL) {
            sscanf(data, "event_id=%d", &id);
            removeEvent(conn, id);
        } else if (strstr(data, "view_participants") != NULL) {
            sscanf(data, "view_participants_event_id=%d", &eventID);
            viewParticipants(conn, eventID);
        }

        printf("Location: /cgi-bin/SEMS.cgi\n\n");
    }
}


void displayForm(struct Event events[], int numEvents) {
    printf("<h1>WELCOME TO SPORTS EVENT MANAGEMENT SYSTEM<h1>");
    printf("<h2>Add Participant</h2>");
    printf("<form method='POST' action='/cgi-bin/SEMS.cgi'>");
    printf("Name: <input type='text' name='participant_name' required><br>");
    printf("Sport: <input type='text' name='participant_sport' required><br>");
    printf("Event: <select name='participant_event_id' required>");
    for (int i = 0; i < numEvents; i++) {
        printf("<option value='%d'>%s</option>", events[i].eventID, events[i].eventName);
    }
    printf("</select><br>");
    printf("<input type='submit' name='add_participant' value='Add Participant'>");
    printf("</form>");

    printf("<h2>Add Event</h2>");
    printf("<form method='POST' action='/cgi-bin/SEMS.cgi'>");
    printf("Event Name: <input type='text' name='event_name' required><br>");
    printf("Date: <input type='date' name='event_date' required><br>");  // Date picker input
    printf("Max Participants: <input type='number' name='max_participants' required><br>");
    printf("<input type='submit' name='add_event' value='Add Event'>");
    printf("</form>");
}


void displayParticipants(struct Participant participants[], int numParticipants) {
    printf("<h2>Participants</h2>");
    printf("<table><tr><th>ID</th><th>Name</th><th>Sport</th><th>Event ID</th><th>Actions</th></tr>");
    for (int i = 0; i < numParticipants; i++) {
        printf("<tr><td>%d</td><td>%s</td><td>%s</td><td>%d</td>"
               "<td><form method='POST' action='/cgi-bin/SEMS.cgi' style='display:inline;'>"
               "<input type='hidden' name='participant_id' value='%d'>"
               "<input type='submit' name='remove_participant' class='remove-button' value='Remove'>"
               "</form></td></tr>",
               participants[i].id, participants[i].name, participants[i].sport, participants[i].eventID, participants[i].id);
    }
    printf("</table>");
}
void viewParticipants(MYSQL *conn, int eventID) {
    char query[256];
    snprintf(query, sizeof(query), "SELECT * FROM participants WHERE eventID = %d", eventID);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Query error (view participants): %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;
    printf("<h2>Participants for Event %d</h2>", eventID);
    printf("<table><tr><th>ID</th><th>Name</th><th>Sport</th><th>Event ID</th></tr>");
    while ((row = mysql_fetch_row(res)) != NULL) {
        printf("<tr><td>%d</td><td>%s</td><td>%s</td><td>%d</td></tr>",
               atoi(row[0]), row[1], row[2], atoi(row[3]));
    }
    printf("</table>");
    mysql_free_result(res);
}


void displayEvents(struct Event events[], int numEvents) {
    printf("<h2>Events</h2>");
    printf("<table><tr><th>Event ID</th><th>Event Name</th><th>Date</th><th>Max Participants</th><th>Current Participants</th><th>Actions</th></tr>");
    for (int i = 0; i < numEvents; i++) {
        printf("<tr><td>%d</td><td>%s</td><td>%s</td><td>%d</td><td>%d</td>"
               "<td><form method='POST' action='/cgi-bin/SEMS.cgi' style='display:inline;'>"
               "<input type='hidden' name='event_id' value='%d'>"
               "<input type='submit' name='remove_event' class='remove-button' value='Remove'>"
               "</form>"
               "<form method='POST' action='/cgi-bin/SEMS.cgi' style='display:inline;'>"
               "<input type='hidden' name='view_participants_event_id' value='%d'>"
               "<input type='submit' name='view_participants' value='View Participants'>"
               "</form></td></tr>",
               events[i].eventID, events[i].eventName, events[i].date, events[i].maxParticipants, events[i].currentParticipants, events[i].eventID, events[i].eventID);
    }
    printf("</table>");
}


#include <ctype.h>
void toUpperCase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper(str[i]);
    }
}

void addParticipant(MYSQL *conn, const char *name, const char *sport, int eventID) {
    char query[512];
    char upperName[100], upperSport[50];

    strncpy(upperName, name, sizeof(upperName) - 1);
    strncpy(upperSport, sport, sizeof(upperSport) - 1);
    toUpperCase(upperName);
    toUpperCase(upperSport);

    snprintf(query, sizeof(query), "SELECT maxParticipants, currentParticipants FROM events WHERE eventID = %d", eventID);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to check event participant limit: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    int maxParticipants = atoi(row[0]);
    int currentParticipants = atoi(row[1]);
    mysql_free_result(res);

    if (currentParticipants >= maxParticipants) {
        printf("<p style='color: red;'>Cannot add participant: Event has reached its maximum limit.</p>");
        return;
    }

    snprintf(query, sizeof(query),
             "INSERT INTO participants (name, sport, eventID) VALUES ('%s', '%s', %d)",
             upperName, upperSport, eventID);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to add participant: %s\n", mysql_error(conn));
    } else {
        snprintf(query, sizeof(query),
                 "UPDATE events SET currentParticipants = currentParticipants + 1 WHERE eventID = %d", eventID);
        mysql_query(conn, query);
        printf("<p style='color: green;'>Participant added successfully!</p>");
    }
}

void addEvent(MYSQL *conn, const char *eventName, const char *date, int maxParticipants) {
    char query[512];
    char upperEventName[100], upperDate[20];


    strncpy(upperEventName, eventName, sizeof(upperEventName) - 1);
    strncpy(upperDate, date, sizeof(upperDate) - 1);
    toUpperCase(upperEventName);
    toUpperCase(upperDate);

    snprintf(query, sizeof(query),
             "INSERT INTO events (eventName, date, maxParticipants, currentParticipants) VALUES ('%s', '%s', %d, 0)",
             upperEventName, upperDate, maxParticipants);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to add event: %s\n", mysql_error(conn));
    }
}


void removeParticipant(MYSQL *conn, int participantID) {
    char query[256];

    snprintf(query, sizeof(query), "SELECT eventID FROM participants WHERE id = %d", participantID);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to get event ID for participant: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    int eventID = atoi(row[0]);
    mysql_free_result(res);

    snprintf(query, sizeof(query), "DELETE FROM participants WHERE id = %d", participantID);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to remove participant: %s\n", mysql_error(conn));
    } else {

        snprintf(query, sizeof(query),
                 "UPDATE events SET currentParticipants = currentParticipants - 1 WHERE eventID = %d", eventID);
        mysql_query(conn, query);

        printf("<p style='color: green;'>Participant removed successfully!</p>");
    }
}



void removeEvent(MYSQL *conn, int eventID) {
    char query[256];
    snprintf(query, sizeof(query), "DELETE FROM events WHERE eventID = %d", eventID);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "Failed to remove event: %s\n", mysql_error(conn));
    }
}


void urlDecode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}
