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

void rebuildIndexes(IndexManager& indexManager, DoctorFile& doctorFile, AppointmentFile& appointmentFile) {

    int doctorsSize = doctorFile.getFileSize();
    for (int offset = HEADER_SIZE; offset + DOCTOR_RECORD_SIZE <= doctorsSize; offset += DOCTOR_RECORD_SIZE) {
        Doctor d;
        if (!doctorFile.readDoctorAt(offset, d)) continue;

        string id = getField(d.id, ID_SIZE);
        indexManager.insertPrimary(indexManager.doctorPrimaryIndex, id, offset);
        indexManager.insertSecondaryIndex(indexManager.doctorNameIndex, getField(d.name, NAME_SIZE), id);
        indexManager.insertSecondaryIndex(indexManager.doctorSpecialtyIndex, getField(d.specialty, SPECIALTY_SIZE), id);
    }


    int appSize = appointmentFile.getFileSize();
    for (int offset = APPOINTMENT_HEADER_SIZE; offset + Appointment::RECORD_SIZE <= appSize; offset += Appointment::RECORD_SIZE) {
        Appointment app;
        if (!appointmentFile.readAppointmentAt(offset, app)) continue;

        string appID = app.getAppointmentID();
        indexManager.insertPrimary(indexManager.appointmentPrimaryIndex, appID, offset);
        indexManager.insertSecondaryIndex(indexManager.appDoctorIdIndex, app.getDoctorID(), appID);
    }
}

void displayMenu() {
    string doctorsFileName = "doctors.dat";
    string appointmentsFileName = "appointments.dat";

    DoctorFile doctorFile(doctorsFileName);
    AppointmentFile appointmentFile(appointmentsFileName);
    IndexManager indexManager("secondary_lists.dat");
    QueryEngine queryEngine(indexManager, doctorFile, appointmentsFileName);

    rebuildIndexes(indexManager, doctorFile, appointmentFile);

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

                if (id.empty()) { cout << ">> Error: Doctor ID cannot be empty.\n"; break; }
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

                if (appID.empty()) { cout << ">> Error: Appointment ID cannot be empty.\n"; break; }
                if (tooLong(appID, Appointment::ID_SIZE, "Appointment ID") ||
                    tooLong(date, Appointment::DATE_SIZE, "Date") ||
                    tooLong(docID, Appointment::DOC_ID_SIZE, "Doctor ID"))
                    break;

                if (indexManager.binarySearchPrimary(indexManager.appointmentPrimaryIndex, appID) != -1) {
                    cout << ">> Error: Appointment ID already exists.\n"; break;
                }
                if (doctorFile.findOffsetById(docID) == -1) {
                    cout << ">> Error: Doctor ID does not exist!\n"; break;
                }

                Appointment app(appID, date, docID);
                int offset = -1;
                if (appointmentFile.addAppointment(app, offset)) {
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
                    cout << ">> Error: Doctor ID not found.\n"; break;
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
                    cout << ">> Error: Old ID not found.\n"; break;
                }
                if (newId.empty() || tooLong(newId, ID_SIZE, "Doctor ID")) break;
                if (doctorFile.findOffsetById(newId) != -1) {
                    cout << ">> Error: New ID already exists.\n"; break;
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
                        if (appOffset != -1) {
                            appointmentFile.updateDoctorIdAt(appOffset, newId);
                        }
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
                    cout << ">> Error: Appointment ID not found.\n"; break;
                }
                if (tooLong(newDate, Appointment::DATE_SIZE, "Date")) break;

                if (appointmentFile.updateDateAt(offset, newDate)) {
                    cout << ">> Appointment date updated successfully.\n";
                }
                break;
            }
            case 11: {
                string appID = readLine("Enter Appointment ID: ");

                int offset = indexManager.binarySearchPrimary(indexManager.appointmentPrimaryIndex, appID);
                if (offset == -1) {
                    cout << ">> Warning: Appointment ID not found.\n"; break;
                }

                Appointment app;
                if (appointmentFile.readAppointmentAt(offset, app)) {
                    string doctorID = app.getDoctorID();
                    if (appointmentFile.deleteAppointmentAt(offset)) {
                        indexManager.removePrimary(indexManager.appointmentPrimaryIndex, appID);
                        indexManager.removeFromSecondary(indexManager.appDoctorIdIndex, doctorID, appID);
                        cout << ">> Appointment deleted successfully.\n";
                    }
                }
                break;
            }
            case 12: {
                string id = readLine("Enter Doctor ID to delete: ");

                Doctor d;
                if (!doctorFile.readDoctorById(id, d)) {
                    cout << ">> Warning: Doctor ID not found.\n"; break;
                }
                string name = getField(d.name, NAME_SIZE);
                string spec = getField(d.specialty, SPECIALTY_SIZE);


                int head = indexManager.binarySearchSecondary(indexManager.appDoctorIdIndex, id);
                vector<string> appIDs = indexManager.getPrimaryKeysFromSecondary(head);
                for (const string& appID : appIDs) {
                    int appOffset = indexManager.binarySearchPrimary(indexManager.appointmentPrimaryIndex, appID);
                    if (appOffset != -1) {
                        if (appointmentFile.deleteAppointmentAt(appOffset)) {
                            indexManager.removePrimary(indexManager.appointmentPrimaryIndex, appID);
                            indexManager.removeFromSecondary(indexManager.appDoctorIdIndex, id, appID);
                        }
                    }
                }

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