#include "Appointment.h"

static string trimRight(const char* field, int size) {
    int len = size;
    while (len > 0 && (field[len - 1] == ' ' || field[len - 1] == '\0'))
        len--;
    return string(field, len);
}

Appointment::Appointment(string id, string date, string docID) {
    memset(appointmentID, ' ', sizeof(appointmentID));
    memset(appointmentDate, ' ', sizeof(appointmentDate));
    memset(doctorID, ' ', sizeof(doctorID));
    setAppointmentID(id);
    setAppointmentDate(date);
    setDoctorID(docID);
}

Appointment::~Appointment() {
}

void Appointment::setAppointmentID(const string& id) {
    memset(appointmentID, ' ', sizeof(appointmentID));
    size_t len = min(id.length(), sizeof(appointmentID));
    memcpy(appointmentID, id.c_str(), len);
}

void Appointment::setAppointmentDate(const string& date) {
    memset(appointmentDate, ' ', sizeof(appointmentDate));
    size_t len = min(date.length(), sizeof(appointmentDate));
    memcpy(appointmentDate, date.c_str(), len);
}

void Appointment::setDoctorID(const string& docID) {
    memset(doctorID, ' ', sizeof(doctorID));
    size_t len = min(docID.length(), sizeof(doctorID));
    memcpy(doctorID, docID.c_str(), len);
}

string Appointment::getAppointmentID() const {
    return trimRight(appointmentID, sizeof(appointmentID));
}

string Appointment::getAppointmentDate() const {
    return trimRight(appointmentDate, sizeof(appointmentDate));
}

string Appointment::getDoctorID() const {
    return trimRight(doctorID, sizeof(doctorID));
}

bool Appointment::updateDate(const string& newDate) {
    if (newDate.length() > sizeof(appointmentDate)) {
        cout << "Error: Date exceeds maximum allowed length (30 chars)." << endl;
        return false;
    }
    setAppointmentDate(newDate);
    return true;
}

int Appointment::deleteRecord(int currentAvailListHead) {
    appointmentID[0] = '*';
    memcpy(appointmentID + 1, &currentAvailListHead, sizeof(int));
    return 1;
}

bool Appointment::isDeleted() const {
    return appointmentID[0] == '*';
}

void Appointment::printAppointment() const {
    cout << "Appointment ID: " << getAppointmentID()
         << " | Date: " << getAppointmentDate()
         << " | Doctor ID: " << getDoctorID() << endl;
}
