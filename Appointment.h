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
    static const int ID_SIZE = 15;
    static const int DATE_SIZE = 30;
    static const int DOC_ID_SIZE = 15;
    static const int RECORD_SIZE = 60;
    char appointmentID[ID_SIZE];
    char appointmentDate[DATE_SIZE];
    char doctorID[DOC_ID_SIZE];

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
    void deleteRecord(int currentAvailListHead);
};

#pragma pack(pop)
class AppointmentFile {
private:
    string fileName;
    int readHeader();
    void writeHeader(int value);
    void writeRecordAt(int offset, const Appointment& a);
public:
    AppointmentFile(string name);
    int getFileSize();
    bool addAppointment(const Appointment& a, int& offset);
    bool deleteAppointmentAt(int offset);
    bool readAppointmentAt(int offset, Appointment& a);
    bool updateDateAt(int offset, const string& newDate);
    bool updateDoctorIdAt(int offset, const string& newDoctorId);
};

#endif
