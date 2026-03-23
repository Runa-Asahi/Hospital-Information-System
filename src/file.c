#include "his.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

int his_console_init(void) {
#ifdef _WIN32
    // Windows 控制台默认常为 CP936(GBK)，源码若为 UTF-8 会导致中文乱码。
    // 切到 UTF-8 以便正确显示 UTF-8 字节序列。
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 尽可能启用 UTF-8 locale；失败则回退到系统默认 locale。
    if (!setlocale(LC_ALL, ".UTF8")) {
        setlocale(LC_ALL, "");
    }

    return 1;
}

// ================== 加载函数 ==================

// 加载患者数据
int load_patients(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0; // 文件不存在，正常返回

    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        // 跳过空行
        if (line[0] == '\n' || line[0] == '\r') continue;
        // 跳过第一行（假设是标题）
        if (line_num == 1) continue;

        int id, age, gender;
        char name[50];
        if (sscanf(line, "%d %d %d %49s", &id, &age, &gender, name) == 4) {
            Patient *p = patient_create(id, name, age, (bool)gender);
            if (p) {
                patient_insert(&g_patient_head, p);
            } else {
                fprintf(stderr, "警告：第%d行患者创建失败（内存不足）\n", line_num);
            }
        } else {
            fprintf(stderr, "警告：第%d行患者格式无效: %s", line_num, line);
        }
    }
    fclose(fp);
    return 1;
}

// 加载医生数据
int load_doctors(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (line[0] == '\n' || line[0] == '\r') continue;
        if (line_num == 1) continue;

        int id, age, gender;
        char name[50], department[50], title[20], schedule[100];
        // schedule 可能包含空格，用 %99[^\n] 读取剩余部分
        if (sscanf(line, "%d %d %d %49s %49s %19s %99[^\n]",
                   &id, &age, &gender, name, department, title, schedule) == 7) {
            Doctor *d = doctor_create(id, name, age, (bool)gender, department, title, schedule);
            if (d) {
                doctor_insert(&g_doctor_head, d);
            } else {
                fprintf(stderr, "警告：第%d行医生创建失败（内存不足）\n", line_num);
            }
        } else {
            fprintf(stderr, "警告：第%d行医生格式无效: %s", line_num, line);
        }
    }
    fclose(fp);
    return 1;
}

// 加载药品数据
int load_drugs(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (line[0] == '\n' || line[0] == '\r') continue;
        if (line_num == 1) continue;

        int id, stock;
        float price;
        char name[50], alias[50], department[50];
        if (sscanf(line, "%d %d %f %49s %49s %49s",
                   &id, &stock, &price, name, alias, department) == 6) {
            Drug *d = drug_create(id, name, alias, stock, price, department);
            if (d) {
                drug_insert(&g_drug_head, d);
            } else {
                fprintf(stderr, "警告：第%d行药品创建失败（内存不足）\n", line_num);
            }
        } else {
            fprintf(stderr, "警告：第%d行药品格式无效: %s", line_num, line);
        }
    }
    fclose(fp);
    return 1;
}

// 加载科室数据
int load_departments(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (line[0] == '\n' || line[0] == '\r') continue;
        if (line_num == 1) continue;

        int id;
        char name[50];
        if (sscanf(line, "%d %49s", &id, name) == 2) {
            Department *d = department_create(id, name);
            if (d) {
                department_insert(&g_department_head, d);
            } else {
                fprintf(stderr, "警告：第%d行科室创建失败（内存不足）\n", line_num);
            }
        } else {
            fprintf(stderr, "警告：第%d行科室格式无效: %s", line_num, line);
        }
    }
    fclose(fp);
    return 1;
}

// 加载病房数据
int load_wards(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (line[0] == '\n' || line[0] == '\r') continue;
        if (line_num == 1) continue;

        int id, total_beds, can_convert;
        char type[20], department[50];
        if (sscanf(line, "%d %19s %d %49s %d",
                   &id, type, &total_beds, department, &can_convert) == 5) {
            Ward *w = ward_create(id, type, total_beds, department, can_convert);
            if (w) {
                ward_insert(&g_ward_head, w);
            } else {
                fprintf(stderr, "警告：第%d行病房创建失败（内存不足）\n", line_num);
            }
        } else {
            fprintf(stderr, "警告：第%d行病房格式无效: %s", line_num, line);
        }
    }
    fclose(fp);
    return 1;
}

// 加载挂号记录
int load_registers(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (line[0] == '\n' || line[0] == '\r') continue;
        if (line_num == 1) continue;

        int id;
        char patient[50], doctor[50], date[20];
        if (sscanf(line, "%d %49s %49s %19s",
                   &id, patient, doctor, date) == 4) {
            Register *r = register_create(id, patient, doctor, date);
            if (r) {
                register_insert(&g_register_head, r);
            } else {
                fprintf(stderr, "警告：第%d行挂号记录创建失败（内存不足）\n", line_num);
            }
        } else {
            fprintf(stderr, "警告：第%d行挂号记录格式无效: %s", line_num, line);
        }
    }
    fclose(fp);
    return 1;
}

// 加载看诊记录
int load_consultations(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[512];
    int line_num = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (line[0] == '\n' || line[0] == '\r') continue;
        if (line_num == 1) continue;

        int id;
        char patient[50], doctor[50], date[20], diagnosis[200], prescription[200];
        // 注意：诊断和处方可能含空格，用 %199[^\n] 读取剩余部分
        if (sscanf(line, "%d %49s %49s %19s %199s %199[^\n]",
                   &id, patient, doctor, date, diagnosis, prescription) == 6) {
            Consultation *c = consultation_create(id, patient, doctor, date, diagnosis, prescription);
            if (c) {
                consultation_insert(&g_consultation_head, c);
            } else {
                fprintf(stderr, "警告：第%d行看诊记录创建失败（内存不足）\n", line_num);
            }
        } else {
            fprintf(stderr, "警告：第%d行看诊记录格式无效: %s", line_num, line);
        }
    }
    fclose(fp);
    return 1;
}

// 加载检查记录
int load_examinations(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (line[0] == '\n' || line[0] == '\r') continue;
        if (line_num == 1) continue;

        int id;
        char patient[50], item[50], date[20];
        float cost;
        if (sscanf(line, "%d %49s %49s %f %19s",
                   &id, patient, item, &cost, date) == 5) {
            Examination *e = examination_create(id, patient, item, cost, date);
            if (e) {
                examination_insert(&g_examination_head, e);
            } else {
                fprintf(stderr, "警告：第%d行检查记录创建失败（内存不足）\n", line_num);
            }
        } else {
            fprintf(stderr, "警告：第%d行检查记录格式无效: %s", line_num, line);
        }
    }
    fclose(fp);
    return 1;
}

// 加载住院记录
int load_inpatients(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (line[0] == '\n' || line[0] == '\r') continue;
        if (line_num == 1) continue;

        int id, ward_id, bed_num;
        float deposit;
        char patient[50], admit_date[20], discharge_date[20];
        if (sscanf(line, "%d %49s %d %d %f %19s %19s",
                   &id, patient, &ward_id, &bed_num, &deposit, admit_date, discharge_date) == 7) {
            Inpatient *ip = inpatient_create(id, patient, ward_id, bed_num, deposit, admit_date, discharge_date);
            if (ip) {
                inpatient_insert(&g_inpatient_head, ip);
            } else {
                fprintf(stderr, "警告：第%d行住院记录创建失败（内存不足）\n", line_num);
            }
        } else {
            fprintf(stderr, "警告：第%d行住院记录格式无效: %s", line_num, line);
        }
    }
    fclose(fp);
    return 1;
}

// ================== 保存函数 ==================

// 保存患者数据
int save_patients(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "id age gender name\n"); // 标题行
    Patient *cur = g_patient_head;
    while (cur) {
        fprintf(fp, "%d %d %d %s\n", cur->id, cur->age, cur->gender, cur->name);
        cur = cur->next;
    }
    fclose(fp);
    return 1;
}

// 保存医生数据
int save_doctors(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "id age gender name department title schedule\n");
    Doctor *cur = g_doctor_head;
    while (cur) {
        fprintf(fp, "%d %d %d %s %s %s %s\n",
                cur->id, cur->age, cur->gender, cur->name,
                cur->department, cur->title, cur->schedule);
        cur = cur->next;
    }
    fclose(fp);
    return 1;
}

// 保存药品数据
int save_drugs(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "id stock price name alias department\n");
    Drug *cur = g_drug_head;
    while (cur) {
        fprintf(fp, "%d %d %.2f %s %s %s\n",
                cur->id, cur->stock, cur->price, cur->name, cur->alias, cur->department);
        cur = cur->next;
    }
    fclose(fp);
    return 1;
}

// 保存科室数据
int save_departments(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "id name\n");
    Department *cur = g_department_head;
    while (cur) {
        fprintf(fp, "%d %s\n", cur->id, cur->name);
        cur = cur->next;
    }
    fclose(fp);
    return 1;
}

// 保存病房数据
int save_wards(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "id type total_beds department can_convert\n");
    Ward *cur = g_ward_head;
    while (cur) {
        fprintf(fp, "%d %s %d %s %d\n", cur->id, cur->type, cur->total_beds, cur->department, cur->can_convert);
        cur = cur->next;
    }
    fclose(fp);
    return 1;
}

// 保存挂号记录
int save_registers(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "id patient_name doctor_name date\n");
    Register *cur = g_register_head;
    while (cur) {
        fprintf(fp, "%d %s %s %s\n", cur->id, cur->patient_name, cur->doctor_name, cur->date);
        cur = cur->next;
    }
    fclose(fp);
    return 1;
}

// 保存看诊记录
int save_consultations(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "id patient_name doctor_name date diagnosis prescription\n");
    Consultation *cur = g_consultation_head;
    while (cur) {
        fprintf(fp, "%d %s %s %s %s %s\n",
                cur->id, cur->patient_name, cur->doctor_name, cur->date,
                cur->diagnosis, cur->prescription);
        cur = cur->next;
    }
    fclose(fp);
    return 1;
}

// 保存检查记录
int save_examinations(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "id patient_name item cost date\n");
    Examination *cur = g_examination_head;
    while (cur) {
        fprintf(fp, "%d %s %s %.2f %s\n", cur->id, cur->patient_name, cur->item, cur->cost, cur->date);
        cur = cur->next;
    }
    fclose(fp);
    return 1;
}

// 保存住院记录
int save_inpatients(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return 0;
    fprintf(fp, "id patient_name ward_id bed_num deposit admit_date discharge_date\n");
    Inpatient *cur = g_inpatient_head;
    while (cur) {
        fprintf(fp, "%d %s %d %d %.2f %s %s\n",
                cur->id, cur->patient_name, cur->ward_id, cur->bed_num, cur->deposit,
                cur->admit_date, cur->discharge_date[0] ? cur->discharge_date : "");
        cur = cur->next;
    }
    fclose(fp);
    return 1;
}

// ================== 统一入口 ==================

int his_init(const char *data_dir) {
    if (!data_dir) return 0;
    (void)his_console_init();
    char path[256];
    int ok = 1;
    sprintf(path, "%s/patients.txt", data_dir);    ok = ok && load_patients(path);
    sprintf(path, "%s/doctors.txt", data_dir);     ok = ok && load_doctors(path);
    sprintf(path, "%s/drugs.txt", data_dir);       ok = ok && load_drugs(path);
    sprintf(path, "%s/departments.txt", data_dir); ok = ok && load_departments(path);
    sprintf(path, "%s/wards.txt", data_dir);       ok = ok && load_wards(path);
    sprintf(path, "%s/registers.txt", data_dir);   ok = ok && load_registers(path);
    sprintf(path, "%s/consultations.txt", data_dir); ok = ok && load_consultations(path);
    sprintf(path, "%s/examinations.txt", data_dir); ok = ok && load_examinations(path);
    sprintf(path, "%s/inpatients.txt", data_dir);  ok = ok && load_inpatients(path);
    return ok;
}

int his_save(const char *data_dir) {
    if (!data_dir) return 0;
    char path[256];
    int ok = 1;
    sprintf(path, "%s/patients.txt", data_dir);    ok = ok && save_patients(path);
    sprintf(path, "%s/doctors.txt", data_dir);     ok = ok && save_doctors(path);
    sprintf(path, "%s/drugs.txt", data_dir);       ok = ok && save_drugs(path);
    sprintf(path, "%s/departments.txt", data_dir); ok = ok && save_departments(path);
    sprintf(path, "%s/wards.txt", data_dir);       ok = ok && save_wards(path);
    sprintf(path, "%s/registers.txt", data_dir);   ok = ok && save_registers(path);
    sprintf(path, "%s/consultations.txt", data_dir); ok = ok && save_consultations(path);
    sprintf(path, "%s/examinations.txt", data_dir); ok = ok && save_examinations(path);
    sprintf(path, "%s/inpatients.txt", data_dir);  ok = ok && save_inpatients(path);
    return ok;
}