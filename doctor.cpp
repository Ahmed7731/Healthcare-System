#include "doctor.h"
#include <fstream>
#include <iostream>
using namespace std;

static void fillField(char* dest, int size, string value) {
    int length = value.length();
    if (length > size) length = size;
    int i = 0;
    for (; i < length; i++) {
        dest[i] = value[i];
    }
    for (; i < size; i++) { // hykml el ba2y b spaces
        dest[i] = ' ';
    }
}

string getField(const char* field, int size)
{
    int lastChar = size;

    while (lastChar > 0 && (field[lastChar - 1] == ' ' || field[lastChar - 1] == '\0')) // msh space wla null
    {
        lastChar--;
    }
    return string(field, lastChar);
}

Doctor createDoctor(string id, string name, string specialty, string address)
{
    Doctor d;
    fillField(d.id, ID_SIZE, id);
    fillField(d.name, NAME_SIZE, name);
    fillField(d.specialty, SPECIALTY_SIZE, specialty);
    fillField(d.address, ADDRESS_SIZE, address);
    return d;
}

DoctorFile::DoctorFile(string name)
{
    fileName = name;

    ifstream check(fileName, ios::binary);
    if (!check)
    {
        ofstream create(fileName, ios::binary);
        int header = -1;
        create.write((char*)&header, sizeof(header));
    }
}

int DoctorFile::readHeader()
{
    ifstream file(fileName, ios::binary);
    int header = -1;
    file.seekg(0, ios::beg);
    file.read((char*)&header, sizeof(int));
    file.close();
    return header;
}

void DoctorFile::writeHeader(int value)
{
    fstream file(fileName, ios::in | ios::out | ios::binary);
    file.seekp(0, ios::beg);
    file.write((char*)&value, sizeof(int));
    file.close();
}

int DoctorFile::getFileSize()
{
    ifstream file(fileName, ios::binary);
    file.seekg(0, ios::end);
    int size = file.tellg();
    file.close();
    return size;
}

void DoctorFile::writeRecordAt(int offset, const Doctor& d)
{
    fstream file(fileName, ios::in | ios::out | ios::binary);
    file.seekp(offset, ios::beg);
    file.write(d.id, ID_SIZE);
    file.write(d.name, NAME_SIZE);
    file.write(d.specialty, SPECIALTY_SIZE);
    file.write(d.address, ADDRESS_SIZE);
    file.close();
}

bool DoctorFile::writeFieldAt(int offset, int fieldStart, int fieldSize, string value)
{
    if ((int)value.length() > fieldSize) return false;

    char buffer[NAME_SIZE];
    fillField(buffer, fieldSize, value);

    fstream file(fileName, ios::in | ios::out | ios::binary);
    file.seekp(offset + fieldStart, ios::beg);
    file.write(buffer, fieldSize);
    file.close();
    return true;
}

int DoctorFile::findOffsetById(string id)
{
    ifstream file(fileName, ios::binary);
    int size = getFileSize();
    int offset = HEADER_SIZE;

    while (offset + DOCTOR_RECORD_SIZE <= size)
    {
        Doctor d;
        file.seekg(offset, ios::beg);
        file.read(d.id, ID_SIZE);

        if (d.id[0] != '*' && getField(d.id, ID_SIZE) == id)
        {
            file.close();
            return offset;
        }
        offset += DOCTOR_RECORD_SIZE;
    }
    file.close();
    return -1;
}

bool DoctorFile::addDoctor(const Doctor& d, int& offset)
{
    string id = getField(d.id, ID_SIZE);
    if (findOffsetById(id) != -1) return false;

    int head = readHeader();

    if (head == -1) offset = getFileSize();
    else
    {
        offset = head;
        int next = -1;
        ifstream file(fileName, ios::binary);
        file.seekg(offset + 1, ios::beg);
        file.read((char*)&next, sizeof(int));
        file.close();

        writeHeader(next);
    }
    writeRecordAt(offset, d);
    return true;
}

bool DoctorFile::deleteDoctorAt(int offset)
{
    Doctor d;
    if (!readDoctorAt(offset, d)) return false;

    int oldHead = readHeader();

    fstream file(fileName, ios::in | ios::out | ios::binary);
    file.seekp(offset, ios::beg);
    char marker = '*';
    file.write(&marker, 1);
    file.write((char*)&oldHead, sizeof(int));
    file.close();

    writeHeader(offset);
    return true;
}

bool DoctorFile::deleteDoctorById(string id, int& offset)
{
    offset = findOffsetById(id);
    if (offset == -1)
    {
        cout << "Warning: Doctor " << id << " does not exist.\n";
        return false;
    }
    return deleteDoctorAt(offset);
}

bool DoctorFile::readDoctorAt(int offset, Doctor& d)
{
    if (offset < HEADER_SIZE || offset + DOCTOR_RECORD_SIZE > getFileSize()) return false;

    ifstream file(fileName, ios::binary);
    file.seekg(offset, ios::beg);
    file.read(d.id, ID_SIZE);
    file.read(d.name, NAME_SIZE);
    file.read(d.specialty, SPECIALTY_SIZE);
    file.read(d.address, ADDRESS_SIZE);
    file.close();

    if (d.id[0] == '*') return false;
    return true;
}

bool DoctorFile::readDoctorById(string id, Doctor& d)
{
    int offset = findOffsetById(id);
    if (offset == -1) return false;
    return readDoctorAt(offset, d);
}

bool DoctorFile::updateName(string id, string newName)
{
    int offset = findOffsetById(id);
    if (offset == -1) return false;
    return writeFieldAt(offset, ID_SIZE, NAME_SIZE, newName);
}

bool DoctorFile::updateSpecialty(string id, string newSpecialty)
{
    int offset = findOffsetById(id);
    if (offset == -1) return false;
    return writeFieldAt(offset, ID_SIZE + NAME_SIZE, SPECIALTY_SIZE, newSpecialty);
}

bool DoctorFile::updateAddress(string id, string newAddress)
{
    int offset = findOffsetById(id);
    if (offset == -1) return false;
    return writeFieldAt(offset, ID_SIZE + NAME_SIZE + SPECIALTY_SIZE, ADDRESS_SIZE, newAddress);
}

bool DoctorFile::updateId(string oldId, string newId, int& offset)
{
    offset = findOffsetById(oldId);
    if (offset == -1) return false;
    if (findOffsetById(newId) != -1) return false;
    return writeFieldAt(offset, 0, ID_SIZE, newId);
}

void DoctorFile::printDoctor(const Doctor& d)
{
    cout << "ID: " << getField(d.id, ID_SIZE)
        << " | Name: " << getField(d.name, NAME_SIZE)
        << " | Specialty: " << getField(d.specialty, SPECIALTY_SIZE)
        << " | Address: " << getField(d.address, ADDRESS_SIZE) << "\n";
}

void DoctorFile::printAllRecords()
{
    cout << "Header (avail head): " << readHeader() << "\n";

    int size = getFileSize();
    for (int offset = HEADER_SIZE; offset + DOCTOR_RECORD_SIZE <= size; offset += DOCTOR_RECORD_SIZE)
    {
        Doctor d;
        if (readDoctorAt(offset, d))
        {
            cout << "[" << offset << "] ";
            printDoctor(d);
        }
        else cout << "[" << offset << "] *deleted*" << "\n";
    }
}
