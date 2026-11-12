#include "Appointment.h"
#include <fstream>
#include <iostream>

const int APPOINTMENT_HEADER_SIZE = 4;
const int APPOINTMENT_DATE_START = 15;
const int APPOINTMENT_DOCTOR_START = 45;

static string trimRight(const char* field, int size) {
    int len = size;
    while (len > 0 && (field[len - 1] == ' ' || field[len - 1] == '\0'))
        len--;
    return string(field, len);
}

static void fillField(char* dest, int size, const string& value) {
    memset(dest, ' ', size);
    size_t len = min(value.length(), (size_t)size);
    memcpy(dest, value.c_str(), len);
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

void Appointment::deleteRecord(int currentAvailListHead) {
    appointmentID[0] = '*';
    memcpy(appointmentID + 1, &currentAvailListHead, sizeof(int));
}

bool Appointment::isDeleted() const {
    return appointmentID[0] == '*';
}

void Appointment::printAppointment() const {
    cout << "Appointment ID: " << getAppointmentID()
         << " | Date: " << getAppointmentDate()
         << " | Doctor ID: " << getDoctorID() << endl;
}


AppointmentFile::AppointmentFile(string name) {
    fileName = name;
    ifstream check(fileName, ios::binary);
    if (!check) {
        ofstream create(fileName, ios::binary);
        int header = -1;
        create.write((char*)&header, sizeof(header));
        create.close();
    }
}
int AppointmentFile::readHeader() {
    ifstream file(fileName, ios::binary);
    int header = -1;
    file.seekg(0, ios::beg);
    file.read((char*)&header, sizeof(int));
    file.close();
    return header;
}
void AppointmentFile::writeHeader(int value) {
    fstream file(fileName, ios::in | ios::out | ios::binary);
    file.seekp(0, ios::beg);
    file.write((char*)&value, sizeof(int));
    file.close();
}
int AppointmentFile::getFileSize() {
    ifstream file(fileName, ios::binary);
    file.seekg(0, ios::end);
    int size = (int)file.tellg();
    file.close();
    return size;
}
void AppointmentFile::writeRecordAt(int offset, const Appointment& a) {
    fstream file(fileName, ios::in | ios::out | ios::binary);
    file.seekp(offset, ios::beg);
    file.write(a.appointmentID, Appointment::ID_SIZE);
    file.write(a.appointmentDate, Appointment::DATE_SIZE);
    file.write(a.doctorID, Appointment::DOC_ID_SIZE);
    file.close();
}
bool AppointmentFile::addAppointment(const Appointment& a, int& offset) {
    int head = readHeader();
    if (head == -1) {
        offset = getFileSize();
    } else {
        offset = head;
        int nextHead = -1;
        ifstream file(fileName, ios::binary);
        file.seekg(offset + 1, ios::beg);
        file.read((char*)&nextHead, sizeof(int));
        file.close();
        writeHeader(nextHead);
    }
    writeRecordAt(offset, a);
    return true;
}

bool AppointmentFile::deleteAppointmentAt(int offset) {
    Appointment a;
    if (!readAppointmentAt(offset, a)) return false;
    int oldHead = readHeader();
    a.deleteRecord(oldHead);
    writeRecordAt(offset, a);
    writeHeader(offset);

    return true;
}

bool AppointmentFile::readAppointmentAt(int offset, Appointment& a) {
    if (offset < APPOINTMENT_HEADER_SIZE || offset + Appointment::RECORD_SIZE > getFileSize())
        return false;
    ifstream file(fileName, ios::binary);
    file.seekg(offset, ios::beg);
    file.read(a.appointmentID, Appointment::ID_SIZE);
    file.read(a.appointmentDate, Appointment::DATE_SIZE);
    file.read(a.doctorID, Appointment::DOC_ID_SIZE);
    bool ok = (bool)file;
    file.close();
    return ok && !a.isDeleted();
}

bool AppointmentFile::updateDateAt(int offset, const string& newDate) {
    if (newDate.length() > Appointment::DATE_SIZE) return false;
    char buffer[Appointment::DATE_SIZE];
    fillField(buffer, Appointment::DATE_SIZE, newDate);
    fstream file(fileName, ios::in | ios::out | ios::binary);
    if (!file) return false;
    file.seekp(offset + APPOINTMENT_DATE_START, ios::beg);
    file.write(buffer, Appointment::DATE_SIZE);
    file.close();
    return true;
}

bool AppointmentFile::updateDoctorIdAt(int offset, const string& newDoctorId) {
    if (newDoctorId.length() > Appointment::DOC_ID_SIZE) return false;
    char buffer[Appointment::DOC_ID_SIZE];
    fillField(buffer, Appointment::DOC_ID_SIZE, newDoctorId);
    fstream file(fileName, ios::in | ios::out | ios::binary);
    if (!file) return false;
    file.seekp(offset + APPOINTMENT_DOCTOR_START, ios::beg);
    file.write(buffer, Appointment::DOC_ID_SIZE);
    file.close();
    return true;
}