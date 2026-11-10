
#ifndef QUERY_ENGINE_H
#define QUERY_ENGINE_H

#include "doctor.h"
#include "Appointment.h"
#include "IndexManager.h"
#include <string>

using namespace std;

class QueryEngine
{
private:
    IndexManager &indexManager;
    DoctorFile &doctorFile;
    string appointmentFileName;

    bool readAppointmentAt(int offset, Appointment &a);
    string trimField(const char *field, int size);
    void printDoctorProjected(const Doctor &d, const string &fields);
    void printAppointmentProjected(const Appointment &a, const string &fields);

public:
    QueryEngine(
        IndexManager &index,
        DoctorFile &doctors,
        string appointmentsFile);

    void executeQuery(string query);

    // queries
    void searchDoctorById(string id, string fields = "all");
    void searchDoctorByName(string name, string fields = "all");
    void searchDoctorBySpecialty(string specialty, string fields = "all");
    void searchDoctorByNameAndSpecialty(string name, string specialty, string fields = "all");
    
    void searchAppointmentById(string id, string fields = "all");
    void searchAppointmentsByDoctorId(string doctorId, string fields = "all");
};

#endif
