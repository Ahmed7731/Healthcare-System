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

// دالة مساعدة لربط وحفظ المواعيد داخل ملف المواعيد الثانوي Binary File
bool addAppointmentToFile(const string& filename, const Appointment& app, int& offset) {
    fstream file(filename, ios::in | ios::out | ios::binary);
    if (!file) {
        // إنشاء الملف إذا لم يكن موجوداً وكتابة Header لـ AvailList
        ofstream create(filename, ios::binary);
        int header = -1;
        create.write((char*)&header, sizeof(int));
        create.close();
        file.open(filename, ios::in | ios::out | ios::binary);
    }

    // قراءة الـ Header الخاص بالـ AvailList
    int head = -1;
    file.seekg(0, ios::beg);
    file.read((char*)&head, sizeof(int));

    if (head == -1) {
        file.seekg(0, ios::end);
        offset = (int)file.tellg();
    } else {
        offset = head;
        file.seekg(offset + 1, ios::beg); // تجاوز علامة '*' لقراءة المكان التالي
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

void displayMenu() {
    string doctorsFileName = "doctors.dat";
    string appointmentsFileName = "appointments.dat";

    DoctorFile doctorFile(doctorsFileName);
    IndexManager indexManager("secondary_lists.dat");
    QueryEngine queryEngine(indexManager, doctorFile, appointmentsFileName);

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
        cin >> choice;
        cin.ignore(); // للتخلص من newline بعد إدخال الرقم

        switch (choice) {
            case 6: { // Add New Doctor
                string id, name, spec, addr;
                cout << "Enter Doctor ID: "; getline(cin, id);
                cout << "Enter Doctor Name: "; getline(cin, name);
                cout << "Enter Specialty: "; getline(cin, spec);
                cout << "Enter Address: "; getline(cin, addr);

                Doctor d = createDoctor(id, name, spec, addr);
                int offset = -1;
                if (doctorFile.addDoctor(d, offset)) {
                    // تحديث الفهارس الرئيسية والثانوية
                    indexManager.doctorPrimaryIndex.push_back({id, offset});
                    indexManager.insertSecondaryIndex(indexManager.doctorNameIndex, name, id);
                    indexManager.insertSecondaryIndex(indexManager.doctorSpecialtyIndex, spec, id);
                    indexManager.sortAllIndexes();
                    cout << ">> Doctor added successfully at offset " << offset << endl;
                } else {
                    cout << ">> Error: Doctor ID already exists or failed to write.\n";
                }
                break;
            }
            case 7: { // Add New Appointment
                string appID, date, docID;
                cout << "Enter Appointment ID: "; getline(cin, appID);
                cout << "Enter Date: "; getline(cin, date);
                cout << "Enter Doctor ID: "; getline(cin, docID);

                // التحقق من وجود الطبيب أولاً
                if (doctorFile.findOffsetById(docID) == -1) {
                    cout << ">> Error: Doctor ID does not exist!\n";
                    break;
                }

                Appointment app(appID, date, docID);
                int offset = -1;
                if (addAppointmentToFile(appointmentsFileName, app, offset)) {
                    indexManager.appointmentPrimaryIndex.push_back({appID, offset});
                    indexManager.insertSecondaryIndex(indexManager.appDoctorIdIndex, docID, appID);
                    indexManager.sortAllIndexes();
                    cout << ">> Appointment added successfully at offset " << offset << endl;
                }
                break;
            }
            case 8: { // Update Doctor Name
                string id, newName;
                cout << "Enter Doctor ID: "; getline(cin, id);
                cout << "Enter New Name: "; getline(cin, newName);

                if (doctorFile.updateName(id, newName)) {
                    cout << ">> Doctor Name updated successfully in file.\n";
                    cout << ">> Note: Re-indexing required for secondary name index.\n";
                } else {
                    cout << ">> Error: Doctor ID not found.\n";
                }
                break;
            }
            case 9: { // Update Doctor ID
                string oldId, newId;
                cout << "Enter Current Doctor ID: "; getline(cin, oldId);
                cout << "Enter New Doctor ID: "; getline(cin, newId);

                int offset = -1;
                if (doctorFile.updateId(oldId, newId, offset)) {
                    cout << ">> Doctor ID updated successfully.\n";
                } else {
                    cout << ">> Error: Old ID not found or New ID already exists.\n";
                }
                break;
            }
            case 10: { // Update Appointment Date
                string appID, newDate;
                cout << "Enter Appointment ID: "; getline(cin, appID);
                cout << "Enter New Date: "; getline(cin, newDate);

                int offset = indexManager.binarySearchPrimary(indexManager.appointmentPrimaryIndex, appID);
                if (offset != -1) {
                    fstream file(appointmentsFileName, ios::in | ios::out | ios::binary);
                    file.seekp(offset + 15, ios::beg); // تخطي الـ ID لكتابة التاريخ الجديد
                    char dateBuf[30];
                    memset(dateBuf, ' ', 30);
                    memcpy(dateBuf, newDate.c_str(), min((size_t)30, newDate.length()));
                    file.write(dateBuf, 30);
                    file.close();
                    cout << ">> Appointment date updated successfully.\n";
                } else {
                    cout << ">> Error: Appointment ID not found.\n";
                }
                break;
            }
            case 11: { // Delete Appointment
                string appID;
                cout << "Enter Appointment ID: "; getline(cin, appID);

                int offset = indexManager.binarySearchPrimary(indexManager.appointmentPrimaryIndex, appID);
                if (offset != -1) {
                    fstream file(appointmentsFileName, ios::in | ios::out | ios::binary);
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

                    cout << ">> Appointment deleted successfully.\n";
                } else {
                    cout << ">> Error: Appointment ID not found.\n";
                }
                break;
            }
            case 12: { // Delete Doctor
                string id;
                cout << "Enter Doctor ID to delete: "; getline(cin, id);

                int offset = -1;
                if (doctorFile.deleteDoctorById(id, offset)) {
                    cout << ">> Doctor deleted successfully.\n";
                } else {
                    cout << ">> Error: Doctor not found.\n";
                }
                break;
            }
            case 13: { // Print Doctor Info
                string id;
                cout << "Enter Doctor ID: "; getline(cin, id);
                queryEngine.searchDoctorById(id);
                break;
            }
            case 14: { // Print Appointment Info
                string appID;
                cout << "Enter Appointment ID: "; getline(cin, appID);
                queryEngine.searchAppointmentById(appID);
                break;
            }
            case 15: { // Write Query
                string queryStr;
                cout << "Enter SQL-like Query: "; getline(cin, queryStr);
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
