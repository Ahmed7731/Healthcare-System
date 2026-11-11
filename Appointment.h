#ifndef APPOINTMENT_H
#define APPOINTMENT_H

#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
using namespace std;
#pragma pack(push, 1)

class Appointment {
public:
    char appointmentID[15];
    char appointmentDate[30];
    char doctorID[15];

    Appointment(string id = "", string date = "", string docID = "");
    ~Appointment();
    void setAppointmentID(const string& id);
    void setAppointmentDate(const string& date);
    void setDoctorID(const string& docID);
    string getAppointmentID() const;
    string getAppointmentDate() const;
    string getDoctorID() const;
    bool isDeleted() const;
    void printAppointment() const;
    bool updateDate(const string& newDate);
    int deleteRecord(int currentAvailListHead);
    static const int RECORD_SIZE = 60;
};

#pragma pack(pop)
#endif
