#include "QueryEngine.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <map>
#include <cctype>

using namespace std;

QueryEngine::QueryEngine(
    IndexManager &index,
    DoctorFile &doctors,
    string appointmentsFile) : indexManager(index),
                               doctorFile(doctors),
                               appointmentFileName(appointmentsFile)
{
}

string QueryEngine::trimField(const char *field, int size)
{
    int len = size;
    while (len > 0 && (field[len - 1] == ' ' || field[len - 1] == '\0'))
        len--;
    return string(field, len);
}

bool QueryEngine::readAppointmentAt(int offset, Appointment &a)
{
    ifstream file(appointmentFileName, ios::binary);
    if (!file)
        return false;

    file.seekg(0, ios::end);
    int fileSize = (int)file.tellg();
    if (offset < HEADER_SIZE || offset + Appointment::RECORD_SIZE > fileSize)
        return false;

    file.seekg(offset, ios::beg);
    file.read(a.appointmentID, 15);
    file.read(a.appointmentDate, 30);
    file.read(a.doctorID, 15);
    if (!file)
        return false;

    return !a.isDeleted();
}

static string toLower(string s)
{
    for (char &c : s)
        c = tolower((unsigned char)c);
    return s;
}

static string trim(const string &s)
{
    size_t a = s.find_first_not_of(" \t\r\n;");
    if (a == string::npos)
        return "";
    size_t b = s.find_last_not_of(" \t\r\n;");
    return s.substr(a, b - a + 1);
}

static bool parseConditions(const string &cond, map<string, string> &out)
{
    size_t i = 0, n = cond.size();
    while (i < n)
    {
        size_t eq = cond.find('=', i);
        if (eq == string::npos)
            return false;
        string field = toLower(trim(cond.substr(i, eq - i)));
        if (field.empty())
            return false;

        size_t j = eq + 1;
        while (j < n && isspace((unsigned char)cond[j]))
            j++;

        string value;
        if (j < n && (cond[j] == '\'' || cond[j] == '"'))
        {
            char q = cond[j++];
            size_t end = cond.find(q, j);
            if (end == string::npos)
                return false;
            value = cond.substr(j, end - j);
            j = end + 1;
        }
        else
        {
            size_t end = j;
            while (end < n && !isspace((unsigned char)cond[end]) && cond[end] != ';')
                end++;
            value = cond.substr(j, end - j);
            j = end;
        }
        out[field] = value;

        while (j < n && (isspace((unsigned char)cond[j]) || cond[j] == ';'))
            j++;
        if (j >= n)
            break;
        if (toLower(cond.substr(j, 3)) != "and")
            return false;
        i = j + 3;
    }
    return !out.empty();
}

// parser
void QueryEngine::executeQuery(string q)
{
    q = trim(q);
    string lq = toLower(q);
    size_t frm = lq.find(" from "), whr = lq.find(" where ");

    if (lq.rfind("select ", 0) != 0 || frm == string::npos || whr == string::npos || whr < frm)
    {
        cout << "Invalid query\n";
        return;
    }

    string fields = toLower(trim(q.substr(7, frm - 7)));
    if (fields == "*" || fields.empty())
        fields = "all";
    string table = toLower(trim(q.substr(frm + 6, whr - frm - 6)));

    map<string, string> c;
    if (!parseConditions(q.substr(whr + 7), c))
    {
        cout << "Invalid condition\n";
        return;
    }

    if (table == "doctors")
    {
        if (c.size() == 1 && c.count("doctor id"))
            searchDoctorById(c["doctor id"], fields);
        else if (c.size() == 1 && c.count("doctor name"))
            searchDoctorByName(c["doctor name"], fields);
        else if (c.size() == 1 && c.count("doctor specialty"))
            searchDoctorBySpecialty(c["doctor specialty"], fields);
        else if (c.size() == 2 && c.count("doctor name") && c.count("doctor specialty"))
            searchDoctorByNameAndSpecialty(c["doctor name"], c["doctor specialty"], fields);
        else
            cout << "Invalid condition on Doctors\n";
    }
    else if (table == "appointments")
    {
        if (c.size() == 1 && c.count("doctor id"))
            searchAppointmentsByDoctorId(c["doctor id"], fields);
        else if (c.size() == 1 && c.count("appointment id"))
            searchAppointmentById(c["appointment id"], fields);
        else
            cout << "Invald condition on Appointments\n";
    }
    else
        cout << "Unknown table: " << table << "\n";
}

// projection
void QueryEngine::printDoctorProjected(const Doctor &d, const string &fields)
{
    if (fields == "all")
    {
        doctorFile.printDoctor(d);
        return;
    }
    if (fields.find("doctor id") != string::npos)
        cout << "ID: " << getField(d.id, ID_SIZE) << "  ";
    if (fields.find("doctor name") != string::npos)
        cout << "Name: " << getField(d.name, NAME_SIZE) << "  ";
    if (fields.find("doctor specialty") != string::npos)
        cout << "Specialty: " << getField(d.specialty, SPECIALTY_SIZE) << "  ";
    if (fields.find("address") != string::npos)
        cout << "Address: " << getField(d.address, ADDRESS_SIZE) << "  ";
    cout << "\n";
}

void QueryEngine::printAppointmentProjected(const Appointment &a, const string &fields)
{
    if (fields == "all")
    {
        cout << "Appointment ID: " << trimField(a.appointmentID, 15)
             << " | Date: " << trimField(a.appointmentDate, 30)
             << " | Doctor ID: " << trimField(a.doctorID, 15) << "\n";
        return;
    }
    if (fields.find("appointment id") != string::npos)
        cout << "Appointment ID: " << trimField(a.appointmentID, 15) << "  ";
    if (fields.find("appointment date") != string::npos)
        cout << "Date: " << trimField(a.appointmentDate, 30) << "  ";
    if (fields.find("doctor id") != string::npos)
        cout << "Doctor ID: " << trimField(a.doctorID, 15) << "  ";
    cout << "\n";
}

// doctors 
void QueryEngine::searchDoctorById(string id, string fields)
{
    int offset = indexManager.binarySearchPrimary(indexManager.doctorPrimaryIndex, id);
    Doctor d;
    if (offset == -1 || !doctorFile.readDoctorAt(offset, d))
    {
        cout << "Doctor not found\n";
        return;
    }
    printDoctorProjected(d, fields);
}

void QueryEngine::searchDoctorByName(string name, string fields)
{
    int head = indexManager.binarySearchSecondary(indexManager.doctorNameIndex, name);
    if (head == -1)
    {
        cout << "No doctors found with this name\n";
        return;
    }

    int count = 0;
    for (const string &id : indexManager.getPrimaryKeysFromSecondary(head))
    {
        int offset = indexManager.binarySearchPrimary(indexManager.doctorPrimaryIndex, id);
        Doctor d;
        if (offset == -1 || !doctorFile.readDoctorAt(offset, d))
            continue;
        if (getField(d.name, NAME_SIZE) != name)
            continue;
        printDoctorProjected(d, fields);
        count++;
    }
    if (count == 0)
        cout << "No doctors found\n";
}

void QueryEngine::searchDoctorBySpecialty(string specialty, string fields)
{
    int head = indexManager.binarySearchSecondary(indexManager.doctorSpecialtyIndex, specialty);
    if (head == -1)
    {
        cout << "No doctors found with this specialty\n";
        return;
    }

    int count = 0;
    for (const string &id : indexManager.getPrimaryKeysFromSecondary(head))
    {
        int offset = indexManager.binarySearchPrimary(indexManager.doctorPrimaryIndex, id);
        Doctor d;
        if (offset == -1 || !doctorFile.readDoctorAt(offset, d))
            continue;
        if (getField(d.specialty, SPECIALTY_SIZE) != specialty)
            continue;
        printDoctorProjected(d, fields);
        count++;
    }
    if (count == 0)
        cout << "No doctors found\n";
}

void QueryEngine::searchDoctorByNameAndSpecialty(string name, string specialty, string fields)
{
    int nameHead = indexManager.binarySearchSecondary(indexManager.doctorNameIndex, name);
    int specHead = indexManager.binarySearchSecondary(indexManager.doctorSpecialtyIndex, specialty);
    if (nameHead == -1 || specHead == -1)
    {
        cout << "No doctors match both condition\n";
        return;
    }

    vector<string> nameIDs = indexManager.getPrimaryKeysFromSecondary(nameHead);
    vector<string> specIDs = indexManager.getPrimaryKeysFromSecondary(specHead);
    unordered_set<string> specSet(specIDs.begin(), specIDs.end());

    int count = 0;
    for (const string &id : nameIDs)
    {
        if (!specSet.count(id))
            continue;
        int offset = indexManager.binarySearchPrimary(indexManager.doctorPrimaryIndex, id);
        Doctor d;
        if (offset == -1 || !doctorFile.readDoctorAt(offset, d))
            continue;
        if (getField(d.name, NAME_SIZE) != name || getField(d.specialty, SPECIALTY_SIZE) != specialty)
            continue;
        printDoctorProjected(d, fields);
        count++;
    }
    if (count == 0)
        cout << "No doctors match both condition\n";
    else
        cout << "Total matching doctors: " << count << "\n";
}

// appointments
void QueryEngine::searchAppointmentById(string id, string fields)
{
    int offset = indexManager.binarySearchPrimary(indexManager.appointmentPrimaryIndex, id);
    Appointment a;
    if (offset == -1 || !readAppointmentAt(offset, a))
    {
        cout << "Appointment not found\n";
        return;
    }
    printAppointmentProjected(a, fields);
}

void QueryEngine::searchAppointmentsByDoctorId(string doctorId, string fields)
{
    int head = indexManager.binarySearchSecondary(indexManager.appDoctorIdIndex, doctorId);
    if (head == -1)
    {
        cout << "No appointments found for this doctor\n";
        return;
    }

    int count = 0;
    for (const string &id : indexManager.getPrimaryKeysFromSecondary(head))
    {
        int offset = indexManager.binarySearchPrimary(indexManager.appointmentPrimaryIndex, id);
        Appointment a;
        if (offset == -1 || !readAppointmentAt(offset, a))
            continue;
        if (trimField(a.doctorID, 15) != doctorId)
            continue;
        printAppointmentProjected(a, fields);
        count++;
    }
    if (count == 0)
        cout << "No appointments found\n";
    else
        cout << "Total appointments: " << count << "\n";
}