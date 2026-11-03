#ifndef DOCTOR_H
#define DOCTOR_H
#include <string>
using namespace std;

const int ID_SIZE = 15;
const int NAME_SIZE = 30;
const int SPECIALTY_SIZE = 20;
const int ADDRESS_SIZE = 30;
const int DOCTOR_RECORD_SIZE = 95;
const int HEADER_SIZE = 4;

struct Doctor
{
    char id[ID_SIZE];
    char name[NAME_SIZE];
    char specialty[SPECIALTY_SIZE];
    char address[ADDRESS_SIZE];
};

Doctor createDoctor(string id, string name, string specialty, string address);
string getField(const char* field, int size);

class DoctorFile
{
private:
    string fileName;

    int readHeader();
    void writeHeader(int value);
    int getFileSize();
    void writeRecordAt(int offset, const Doctor& d);
    bool writeFieldAt(int offset, int fieldStart, int fieldSize, string value);

public:
    DoctorFile(string name);

    int findOffsetById(string id);

    bool addDoctor(const Doctor& d, int& offset);

    bool deleteDoctorAt(int offset);
    bool deleteDoctorById(string id, int& offset);

    bool readDoctorAt(int offset, Doctor& d);
    bool readDoctorById(string id, Doctor& d);

    bool updateName(string id, string newName);
    bool updateSpecialty(string id, string newSpecialty);
    bool updateAddress(string id, string newAddress);

    bool updateId(string oldId, string newId, int& offset);

    void printAllRecords();
    void printDoctor(const Doctor& d);
};
#endif
