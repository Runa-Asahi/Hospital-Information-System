#ifndef HIS_H
#define HIS_H

#include <stdbool.h>

// 患者
typedef struct Patient {
    int id;                     // 唯一ID
    char name[50];
    int age;
    bool gender;                // 0:女 1:男
    struct Patient *prev;
    struct Patient *next;
} Patient;

// 医生
typedef struct Doctor {
    int id;                     // 工号
    char name[50];
    int age;
    bool gender;
    char department[50];
    char title[20];
    char schedule[100];         // 出诊时间
    struct Doctor *prev;
    struct Doctor *next;
} Doctor;

// 药品
typedef struct Drug {
    int id;
    char name[50];              // 通用名
    char alias[50];             // 别名
    int stock;                  // 库存
    float price;                // 单价
    char department[50];        // 关联科室
    struct Drug *prev;
    struct Drug *next;
} Drug;

// 科室
typedef struct Department {
    int id;
    char name[50];
    struct Department *prev;
    struct Department *next;
} Department;

// 病房
typedef struct Ward {
    int id;
    char type[20];              // 病房类型（单人、双人、重症等）
    int total_beds;
    int occupied_beds;
    char department[50];        // 关联科室
    int can_convert;            // 是否可转换
    struct Ward *prev;
    struct Ward *next;
} Ward;

// 挂号记录
typedef struct Register {
    int id;                     // 挂号ID
    char patient_name[50];
    char doctor_name[50];
    char date[20];              // YYYY-MM-DD
    struct Register *prev;
    struct Register *next;
} Register;

// 看诊记录
typedef struct Consultation {
    int id;
    char patient_name[50];
    char doctor_name[50];
    char date[20];
    char diagnosis[200];// 诊断结果
    char prescription[200];// 处方
    struct Consultation *prev;
    struct Consultation *next;
} Consultation;

// 检查记录
typedef struct Examination {
    int id;
    char patient_name[50];
    char item[50];// 检查项目
    float cost;
    char date[20];
    struct Examination *prev;
    struct Examination *next;
} Examination;

// 住院记录
typedef struct Inpatient {
    int id;
    char patient_name[50];
    int ward_id;
    int bed_num;
    float deposit;// 押金
    char admit_date[20];// 入院日期
    char discharge_date[20];    // 未出院可为空
    struct Inpatient *prev;
    struct Inpatient *next;
} Inpatient;

extern Patient *g_patient_head;
extern Doctor  *g_doctor_head;
extern Drug    *g_drug_head;
extern Department *g_department_head;
extern Ward    *g_ward_head;
extern Register *g_register_head;
extern Consultation *g_consultation_head;
extern Examination *g_examination_head;
extern Inpatient *g_inpatient_head;

/* ==================== 文件读写/初始化 ==================== */
int load_patients(const char *filename);
int load_doctors(const char *filename);
int load_drugs(const char *filename);
int load_departments(const char *filename);
int load_wards(const char *filename);
int load_registers(const char *filename);
int load_consultations(const char *filename);
int load_examinations(const char *filename);
int load_inpatients(const char *filename);

int save_patients(const char *filename);
int save_doctors(const char *filename);
int save_drugs(const char *filename);
int save_departments(const char *filename);
int save_wards(const char *filename);
int save_registers(const char *filename);
int save_consultations(const char *filename);
int save_examinations(const char *filename);
int save_inpatients(const char *filename);

int his_init(const char *data_dir);
int his_save(const char *data_dir);

/* ==================== 患者模块 ==================== */
Patient* patient_create(int id, const char *name, int age, bool gender);
int patient_insert(Patient **head, Patient *node);
int patient_delete(Patient **head, int id);
Patient* patient_find_by_id(Patient *head, int id);
Patient* patient_find_by_name(Patient *head, const char *name);
int patient_update(Patient *p, const char *name, int age, bool gender);
int patient_print_all(Patient *head);
int patient_free_all(Patient **head);

/* ==================== 医生模块 ==================== */
Doctor* doctor_create(int id, const char *name, int age, bool gender,
                      const char *department, const char *title,
                      const char *schedule);
int doctor_insert(Doctor **head, Doctor *node);
int doctor_delete(Doctor **head, int id);
Doctor* doctor_find_by_id(Doctor *head, int id);
Doctor* doctor_find_by_name(Doctor *head, const char *name);
int doctor_update(Doctor *d, const char *name, int age, bool gender,
                   const char *department, const char *title,
                   const char *schedule);
int doctor_print_all(Doctor *head);
int doctor_free_all(Doctor **head);

/* ==================== 药品模块 ==================== */
Drug* drug_create(int id, const char *name, const char *alias, int stock, float price, const char *department);
int drug_insert(Drug **head, Drug *node);
int drug_delete(Drug **head, int id);
Drug* drug_find_by_id(Drug *head, int id);
Drug* drug_find_by_name(Drug *head, const char *name);
int drug_update_stock(Drug *d, int delta);   // delta可正可负
int drug_print_all(Drug *head);
int drug_free_all(Drug **head);

/* ==================== 科室模块 ==================== */
Department* department_create(int id, const char *name);
int department_insert(Department **head, Department *node);
int department_delete(Department **head, int id);
Department* department_find_by_id(Department *head, int id);
Department* department_find_by_name(Department *head, const char *name);
int department_print_all(Department *head);
int department_free_all(Department **head);

/* ==================== 病房模块 ==================== */
Ward* ward_create(int id, const char *type, int total_beds, const char *department, int can_convert);
int ward_insert(Ward **head, Ward *node);
int ward_delete(Ward **head, int id);
Ward* ward_find_by_id(Ward *head, int id);
Ward* ward_find_by_type(Ward *head, const char *type); // 返回第一个匹配
int ward_has_free_bed(Ward *w);
int ward_occupy_bed(Ward *w);
int ward_release_bed(Ward *w);
int ward_print_all(Ward *head);
int ward_free_all(Ward **head);

/* ==================== 挂号模块 ==================== */
Register* register_create(int id, const char *patient, const char *doctor, const char *date);
int register_insert(Register **head, Register *node);
int register_delete(Register **head, int id);
Register* register_find_by_id(Register *head, int id);
int register_count_by_doctor_date(Register *head, const char *doctor, const char *date);
int register_print_by_patient(Register *head, const char *patient);
int register_print_all(Register *head);
int register_free_all(Register **head);

/* ==================== 看诊模块 ==================== */
Consultation* consultation_create(int id, const char *patient, const char *doctor,
                                  const char *date, const char *diagnosis, const char *prescription);
int consultation_insert(Consultation **head, Consultation *node);
int consultation_delete(Consultation **head, int id);
Consultation* consultation_find_by_id(Consultation *head, int id);
int consultation_print_by_patient(Consultation *head, const char *patient);
int consultation_free_all(Consultation **head);

/* ==================== 检查模块 ==================== */
Examination* examination_create(int id, const char *patient, const char *item, float cost, const char *date);
int examination_insert(Examination **head, Examination *node);
int examination_delete(Examination **head, int id);
Examination* examination_find_by_id(Examination *head, int id);
int examination_print_by_patient(Examination *head, const char *patient);
int examination_free_all(Examination **head);

/* ==================== 住院模块 ==================== */
Inpatient* inpatient_create(int id, const char *patient, int ward_id, int bed_num,
                            float deposit, const char *admit_date, const char *discharge_date);
int inpatient_insert(Inpatient **head, Inpatient *node);
int inpatient_delete(Inpatient **head, int id);
Inpatient* inpatient_find_by_id(Inpatient *head, int id);
Inpatient* inpatient_find_by_patient(Inpatient *head, const char *patient); // 返回第一个
int inpatient_update_discharge(Inpatient *ip, const char *discharge_date);
int inpatient_print_all(Inpatient *head);
int inpatient_free_all(Inpatient **head);

#endif // HIS_H