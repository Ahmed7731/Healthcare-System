#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <algorithm>
#include "doctor.h"
#include "Appointment.h"
#include "IndexManager.h"
#include "QueryEngine.h"

using namespace std;

const int APPOINTMENT_HEADER_SIZE = 4;
const int APPOINTMENT_DATE_START = 15;
const int APPOINTMENT_DOCTOR_START = 45;

string readLine(string message) {
    string text;
    cout << message;
    getline(cin, text);

    size_t first = text.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

bool tooLong(const string& value, int maxSize, const string& fieldName) {
    if ((int)value.length() > maxSize) {
        cout << ">> Error: " << fieldName << " is longer than " << maxSize << " characters.\n";
        return true;
    }
    return false;
}

bool addAppointmentToFile(const string& filename, const Appointment& app, int& offset) {
    fstream file(filename, ios::in | ios::out | ios::binary);
    if (!file) {
        ofstream create(filename, ios::binary);
        int header = -1;
        create.write((char*)&header, sizeof(int));
        create.close();
        file.open(filename, ios::in | ios::out | ios::binary);
    }

    int head = -1;
    file.seekg(0, ios::beg);
    file.read((char*)&head, sizeof(int));

    if (head == -1) {
        file.seekg(0, ios::end);
        offset = (int)file.tellg();
    } else {
        offset = head;
        file.seekg(offset + 1, ios::beg);
        int nextHead = -1;
        file.read((char*)&nextHead, sizeof(int));

        file.seekp(0, ios::beg);
        file.write((char*)&nextHead, sizeof(int));
    }

    file.seekp(offset, ios::beg);
    file.write(app.appointmentID, 15);
    file.write(app.appointmentDate, 30);
    file.write(app.doctorID, 15);
    file.close();

    return true;
}

bool readAppointmentRecord(const string& filename, int offset, Appointment& app) {
    ifstream file(filename, ios::binary);
    file.seekg(offset, ios::beg);
    file.read(app.appointmentID, 15);
    file.read(app.appointmentDate, 30);
    file.read(app.doctorID, 15);
    bool ok = (bool)file;
    file.close();
    return ok && !app.isDeleted();
}

void writeAppointmentField(const string& filename, int offset, int fieldStart, int fieldSize, const string& value) {
    char buffer[30];
    memset(buffer, ' ', fieldSize);
    memcpy(buffer, value.c_str(), min((size_t)fieldSize, value.length()));

    fstream file(filename, ios::in | ios::out | ios::binary);
    file.seekp(offset + fieldStart, ios::beg);
    file.write(buffer, fieldSize);
    file.close();
}

// Marks the appointment as deleted in the file (AVAIL LIST) and removes it from all indexes.
bool deleteAppointment(IndexManager& indexManager, const string& filename, const string& appID) {
    int offset = indexManager.binarySearchPrimary(indexManager.appointmentPrimaryIndex, appID);
    if (offset == -1) return false;

    Appointment app;
    if (!readAppointmentRecord(filename, offset, app)) return false;
    string doctorID = app.getDoctorID();

    fstream file(filename, ios::in | ios::out | ios::binary);
    int oldHead = -1;
    file.seekg(0, ios::beg);
    file.read((char*)&oldHead, sizeof(int));

    file.seekp(offset, ios::beg);
    char mark = '*';
    file.write(&mark, 1);
    file.write((char*)&oldHead, sizeof(int));

    file.seekp(0, ios::beg);
    file.write((char*)&offset, sizeof(int));
    file.close();

    indexManager.removePrimary(indexManager.appointmentPrimaryIndex, appID);
    indexManager.removeFromSecondary(indexManager.appDoctorIdIndex, doctorID, appID);
    return true;
}

// The indexes are in memory, so they are rebuilt from the data files when the program starts.
void rebuildIndexes(IndexManager& indexManager, DoctorFile& doctorFile, const string& appointmentsFileName) {
    int doctorsSize = doctorFile.getFileSize();
    for (int offset = HEADER_SIZE; offset + DOCTOR_RECORD_SIZE <= doctorsSize; offset += DOCTOR_RECORD_SIZE) {
        Doctor d;
        if (!doctorFile.readDoctorAt(offset, d)) continue;

        string id = getField(d.id, ID_SIZE);
        indexManager.insertPrimary(indexManager.doctorPrimaryIndex, id, offset);
        indexManager.insertSecondaryIndex(indexManager.doctorNameIndex, getField(d.name, NAME_SIZE), id);
        indexManager.insertSecondaryIndex(indexManager.doctorSpecialtyIndex, getField(d.specialty, SPECIALTY_SIZE), id);
    }

    ifstream file(appointmentsFileName, ios::binary | ios::ate);
    if (!file) return;
    int appointmentsSize = (int)file.tellg();
    file.close();

    for (int offset = APPOINTMENT_HEADER_SIZE; offset + Appointment::RECORD_SIZE <= appointmentsSize; offset += Appointment::RECORD_SIZE) {
        Appointment app;
        if (!readAppointmentRecord(appointmentsFileName, offset, app)) continue;

        string appID = app.getAppointmentID();
        indexManager.insertPrimary(indexManager.appointmentPrimaryIndex, appID, offset);
        indexManager.insertSecondaryIndex(indexManager.appDoctorIdIndex, app.getDoctorID(), appID);
    }
}

void displayMenu() {
    string doctorsFileName = "doctors.dat";
    string appointmentsFileName = "appointments.dat";

    DoctorFile doctorFile(doctorsFileName);
    IndexManager indexManager("secondary_lists.dat");
    QueryEngine queryEngine(indexManager, doctorFile, appointmentsFileName);

    rebuildIndexes(indexManager, doctorFile, appointmentsFileName);

    int choice = 0;

    while (choice != 16) {
        cout << "\n=========================================\n";
        cout << "       Healthcare Management System      \n";
        cout << "=========================================\n";
        cout << "6.  Add New Doctor\n";
        cout << "7.  Add New Appointment\n";
        cout << "8.  Update Doctor Name (Doctor ID)\n";
        cout << "9.  Update Doctor ID [Key Update Test]\n";
        cout << "10. Update Appointment Date (Appointment ID)\n";
        cout << "11. Delete Appointment (Appointment ID)\n";
        cout << "12. Delete Doctor (Doctor ID)\n";
        cout << "13. Print Doctor Info (Doctor ID)\n";
        cout << "14. Print Appointment Info (Appointment ID)\n";
        cout << "15. Write Query\n";
        cout << "16. Exit\n";
        cout << "=========================================\n";
        cout << "Enter your choice (6-16): ";

        if (!(cin >> choice)) {
            if (cin.eof()) break;
            cin.clear();
            choice = 0;
        }
        cin.ignore(10000, '\n');

        switch (choice) {
            case 6: {
                string id = readLine("Enter Doctor ID: ");
                string name = readLine("Enter Doctor Name: ");
                string spec = readLine("Enter Specialty: ");
                string addr = readLine("Enter Address: ");

                if (id.empty()) {
                    cout << ">> Error: Doctor ID cannot be empty.\n";
                    break;
                }
                if (tooLong(id, ID_SIZE, "Doctor ID") || tooLong(name, NAME_SIZE, "Doctor Name") ||
                    tooLong(spec, SPECIALTY_SIZE, "Specialty") || tooLong(addr, ADDRESS_SIZE, "Address"))
                    break;

                Doctor d = createDoctor(id, name, spec, addr);
                int offset = -1;
                if (doctorFile.addDoctor(d, offset)) {
                    indexManager.insertPrimary(indexManager.doctorPrimaryIndex, id, offset);
                    indexManager.insertSecondaryIndex(indexManager.doctorNameIndex, name, id);
                    indexManager.insertSecondaryIndex(indexManager.doctorSpecialtyIndex, spec, id);
                    cout << ">> Doctor added successfully at offset " << offset << endl;
                } else {
                    cout << ">> Error: Doctor ID already exists or failed to write.\n";
                }
                break;
            }
            case 7: {
                string appID = readLine("Enter Appointment ID: ");
                string date = readLine("Enter Date: ");
                string docID = readLine("Enter Doctor ID: ");

                if (appID.empty()) {
                    cout << ">> Error: Appointment ID cannot be empty.\n";
                    break;
                }
                if (tooLong(appID, 15, "Appointment ID") || tooLong(date, 30, "Date") || tooLong(docID, 15, "Doctor ID"))
                    break;

                if (indexManager.binarySearchPrimary(indexManager.appointmentPrimaryIndex, appID) != -1) {
                    cout << ">> Error: Appointment ID already exists.\n";
                    break;
                }
                if (doctorFile.findOffsetById(docID) == -1) {
                    cout << ">> Error: Doctor ID does not exist!\n";
                    break;
                }

                Appointment app(appID, date, docID);
                int offset = -1;
                if (addAppointmentToFile(appointmentsFileName, app, offset)) {
                    indexManager.insertPrimary(indexManager.appointmentPrimaryIndex, appID, offset);
                    indexManager.insertSecondaryIndex(indexManager.appDoctorIdIndex, docID, appID);
                    cout << ">> Appointment added successfully at offset " << offset << endl;
                }
                break;
            }
            case 8: {
                string id = readLine("Enter Doctor ID: ");
                string newName = readLine("Enter New Name: ");

                Doctor d;
                if (!doctorFile.readDoctorById(id, d)) {
                    cout << ">> Error: Doctor ID not found.\n";
                    break;
                }
                if (tooLong(newName, NAME_SIZE, "Doctor Name")) break;

                string oldName = getField(d.name, NAME_SIZE);
                if (doctorFile.updateName(id, newName)) {
                    indexManager.removeFromSecondary(indexManager.doctorNameIndex, oldName, id);
                    indexManager.insertSecondaryIndex(indexManager.doctorNameIndex, newName, id);
                    cout << ">> Doctor Name updated successfully.\n";
                }
                break;
            }
            case 9: {
                string oldId = readLine("Enter Current Doctor ID: ");
                string newId = readLine("Enter New Doctor ID: ");

                Doctor d;
                if (!doctorFile.readDoctorById(oldId, d)) {
                    cout << ">> Error: Old ID not found.\n";
                    break;
                }
                if (newId.empty() || tooLong(newId, ID_SIZE, "Doctor ID")) break;
                if (doctorFile.findOffsetById(newId) != -1) {
                    cout << ">> Error: New ID already exists.\n";
                    break;
                }

                int offset = -1;
                if (doctorFile.updateId(oldId, newId, offset)) {
                    indexManager.changePrimaryKey(indexManager.doctorPrimaryIndex, oldId, newId);
                    indexManager.changePrimaryKeyInSecondary(indexManager.doctorNameIndex, getField(d.name, NAME_SIZE), oldId, newId);
                    indexManager.changePrimaryKeyInSecondary(indexManager.doctorSpecialtyIndex, getField(d.specialty, SPECIALTY_SIZE), oldId, newId);

                    int head = indexManager.binarySearchSecondary(indexManager.appDoctorIdIndex, oldId);
                    vector<string> appIDs = indexManager.getPrimaryKeysFromSecondary(head);
                    for (const string& appID : appIDs) {
                        int appOffset = indexManager.binarySearchPrimary(indexManager.appointmentPrimaryIndex, appID);
                        writeAppointmentField(appointmentsFileName, appOffset, APPOINTMENT_DOCTOR_START, 15, newId);
                    }
                    indexManager.renameSecondaryKey(indexManager.appDoctorIdIndex, oldId, newId);

                    cout << ">> Doctor ID updated successfully (" << appIDs.size() << " appointment(s) updated too).\n";
                }
                break;
            }
            case 10: {
                string appID = readLine("Enter Appointment ID: ");
                string newDate = readLine("Enter New Date: ");

                int offset = indexManager.binarySearchPrimary(indexManager.appointmentPrimaryIndex, appID);
                if (offset == -1) {
                    cout << ">> Error: Appointment ID not found.\n";
                    break;
                }
                if (tooLong(newDate, 30, "Date")) break;

                writeAppointmentField(appointmentsFileName, offset, APPOINTMENT_DATE_START, 30, newDate);
                cout << ">> Appointment date updated successfully.\n";
                break;
            }
            case 11: {
                string appID = readLine("Enter Appointment ID: ");

                if (deleteAppointment(indexManager, appointmentsFileName, appID))
                    cout << ">> Appointment deleted successfully.\n";
                else
                    cout << ">> Warning: Appointment ID not found.\n";
                break;
            }
            case 12: {
                string id = readLine("Enter Doctor ID to delete: ");

                Doctor d;
                if (!doctorFile.readDoctorById(id, d)) {
                    cout << ">> Warning: Doctor ID not found.\n";
                    break;
                }
                string name = getField(d.name, NAME_SIZE);
                string spec = getField(d.specialty, SPECIALTY_SIZE);

                int head = indexManager.binarySearchSecondary(indexManager.appDoctorIdIndex, id);
                vector<string> appIDs = indexManager.getPrimaryKeysFromSecondary(head);
                for (const string& appID : appIDs)
                    deleteAppointment(indexManager, appointmentsFileName, appID);

                int offset = -1;
                if (doctorFile.deleteDoctorById(id, offset)) {
                    indexManager.removePrimary(indexManager.doctorPrimaryIndex, id);
                    indexManager.removeFromSecondary(indexManager.doctorNameIndex, name, id);
                    indexManager.removeFromSecondary(indexManager.doctorSpecialtyIndex, spec, id);
                    cout << ">> Doctor deleted successfully (" << appIDs.size() << " appointment(s) deleted too).\n";
                }
                break;
            }
            case 13: {
                string id = readLine("Enter Doctor ID: ");
                queryEngine.searchDoctorById(id);
                break;
            }
            case 14: {
                string appID = readLine("Enter Appointment ID: ");
                queryEngine.searchAppointmentById(appID);
                break;
            }
            case 15: {
                string queryStr = readLine("Enter SQL-like Query: ");
                queryEngine.executeQuery(queryStr);
                break;
            }
            case 16:
                cout << "\nExiting System. Goodbye!\n";
                break;
            default:
                cout << "\nInvalid choice! Please enter a number between 6 and 16.\n";
                break;
        }
    }
}

int main() {
    displayMenu();
    return 0;
}
