#ifndef HIS_H
#define HIS_H

#include <stdbool.h>

/*
 * Hospital Information System (HIS)
 * 说明：本头文件定义系统核心数据结构、全局链表表头，以及各模块对外接口声明。
 * 数据结构约定：各表均为双向链表（prev/next），由对应模块维护与释放。
 * 持久化约定：data/ 目录下各类 .txt 文件首行为标题行；加载时跳过，保存时写回。
 */

/** 患者信息（Patient）。 */
typedef struct Patient {
    int id;                     // 唯一ID
    char name[50];
    int age;
    bool gender;                // 0:女 1:男
    struct Patient *prev;
    struct Patient *next;
} Patient;

/** 医生信息（Doctor）。 */
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

/** 药品信息（Drug）。 */
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

/** 科室信息（Department）。 */
typedef struct Department {
    int id;
    char name[50];
    struct Department *prev;
    struct Department *next;
} Department;

/** 病房信息（Ward）。 */
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

/** 挂号记录（Register）。 */
typedef struct Register {
    int id;                     // 挂号ID
    char patient_name[50];
    char doctor_name[50];
    char date[20];              // YYYY-MM-DD
    struct Register *prev;
    struct Register *next;
} Register;

/** 看诊记录（Consultation）。 */
typedef struct Consultation {
    int id;
    char patient_name[50];
    char doctor_name[50];
    char date[20];
    char diagnosis[200];         // 诊断结果
    char prescription[200];      // 处方
    struct Consultation *prev;
    struct Consultation *next;
} Consultation;

/** 检查记录（Examination）。 */
typedef struct Examination {
    int id;
    char patient_name[50];
    char item[50];               // 检查项目
    float cost;
    char date[20];
    struct Examination *prev;
    struct Examination *next;
} Examination;

/** 住院记录（Inpatient）。deposit 字段作为“押金余额”。 */
typedef struct Inpatient {
    int id;                      // 住院ID
    char patient_name[50];       // 患者姓名
    int ward_id;                 // 病房ID
    int bed_num;                 // 床位号
    float deposit;               // 押金余额
    char admit_date[20];         // 入院日期/时间
    char discharge_date[20];    // 未出院可为空
    struct Inpatient *prev;
    struct Inpatient *next;
} Inpatient;

/** 计费流水类型（BillingType）。 */
typedef enum BillingType {
    BILL_DEPOSIT = 0,   // 入院押金
    BILL_TOPUP   = 1,   // 补缴押金
    BILL_CHARGE  = 2,   // 费用扣除
    BILL_REFUND  = 3    // 退费
} BillingType;

/** 计费流水记录（Billing）。 */
typedef struct Billing {
    int id;                 // 流水ID
    int inpatient_id;       // 关联住院ID
    char patient_name[50];  // 患者姓名（便于打印）
    int ward_id;            // 病房ID（便于追溯）
    BillingType type;       // 流水类型
    float amount;           // 金额（正数）
    char datetime[24];      // 时间字符串：YYYY-MM-DD / YYYY-MM-DDTHH:MM
    char note[100];         // 备注（可为空）
    struct Billing *prev;
    struct Billing *next;
} Billing;

/**
 * 全局链表表头：由各模块维护。
 * @note 仅声明，定义位于各自 .c 文件中。
 */
extern Patient *g_patient_head;
extern Doctor  *g_doctor_head;
extern Drug    *g_drug_head;
extern Department *g_department_head;
extern Ward    *g_ward_head;
extern Register *g_register_head;
extern Consultation *g_consultation_head;
extern Examination *g_examination_head;
extern Inpatient *g_inpatient_head;
extern Billing *g_billing_head;

/* ==================== 控制台编码 ==================== */
/**
 * 初始化控制台编码/Locale（主要用于 Windows 下 UTF-8 中文输出）。
 * @return 成功返回 1
 */
int his_console_init(void);

/* ==================== 程序入口（控制台/GUI） ==================== */
/**
 * 运行原有的控制台菜单循环。
 * @return 进程返回码
 */
int his_console_main(void);

/**
 * 在“已初始化且已加载数据”的前提下，执行一次控制台菜单操作（1~9）。
 * 该函数会在控制台中进行交互输入/输出。
 * @return 成功返回 1；失败返回 0
 */
int his_console_run_operation(int op);

/**
 * 释放所有全局链表节点（退出时调用）。
 */
void his_release_all(void);

/**
 * 运行 Win32 图形化主界面（Windows 平台）。
 * @return 进程返回码
 */
int his_gui_main(void);

/* ==================== 文件读写/初始化 ==================== */
/**
 * 加载/保存函数：把 data/ 目录下各类 .txt 文件映射到全局链表。
 * @return 成功返回 1；失败返回 0
 */
int load_patients(const char *filename);
int load_doctors(const char *filename);
int load_drugs(const char *filename);
int load_departments(const char *filename);
int load_wards(const char *filename);
int load_registers(const char *filename);
int load_consultations(const char *filename);
int load_examinations(const char *filename);
int load_inpatients(const char *filename);
int load_billings(const char *filename);

int save_patients(const char *filename);
int save_doctors(const char *filename);
int save_drugs(const char *filename);
int save_departments(const char *filename);
int save_wards(const char *filename);
int save_registers(const char *filename);
int save_consultations(const char *filename);
int save_examinations(const char *filename);
int save_inpatients(const char *filename);
int save_billings(const char *filename);

/**
 * 系统初始化：按 data_dir 加载各表。
 */
int his_init(const char *data_dir);

/**
 * 系统保存：按 data_dir 保存各表。
 */
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
int ward_free_beds(const Ward *w);
Ward* ward_find_available_by_type(Ward *head, const char *type, int required_free_beds);
Ward* ward_find_by_type(Ward *head, const char *type); // 同类型中择优（至少1张空床）
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
int consultation_print_all(Consultation *head);
/**
 * 题签：规范化修改流程（撤销+补录）。
 * 说明：撤销旧记录（打印原因），避免直接“改写历史”。
 * @return 成功撤销返回 1；失败返回 0
 */
int consultation_revoke(Consultation *head, int id, const char *reason);
int consultation_free_all(Consultation **head);

/* ==================== 检查模块 ==================== */
Examination* examination_create(int id, const char *patient, const char *item, float cost, const char *date);
int examination_insert(Examination **head, Examination *node);
int examination_delete(Examination **head, int id);
Examination* examination_find_by_id(Examination *head, int id);
int examination_print_by_patient(Examination *head, const char *patient);
int examination_print_all(Examination *head);
int examination_free_all(Examination **head);

/* ==================== 汇总/统计（题签扩展查询） ==================== */
/** 患者维度：打印该患者所有业务记录（挂号/看诊/检查/住院/计费）。 */
int his_print_patient_history(const char *patient_name);

/** 医生维度：按医生工号汇总（挂号数量/看诊数量；可选按日期）。 */
int his_summary_by_doctor_id(int doctor_id, const char *date_opt);

/** 科室维度：按科室名称汇总（按医生所属科室归集挂号/看诊数量）。 */
int his_summary_by_department(const char *department_name, const char *start_date_opt, const char *end_date_opt);

/** 时间范围维度：汇总全院业务记录数量（挂号/看诊/检查/住院/计费等）。 */
int his_summary_by_date_range(const char *start_date, const char *end_date);

/* ==================== 住院模块 ==================== */
Inpatient* inpatient_create(int id, const char *patient, int ward_id, int bed_num,
                            float deposit, const char *admit_date, const char *discharge_date);
int inpatient_insert(Inpatient **head, Inpatient *node);
int inpatient_delete(Inpatient **head, int id);
int inpatient_next_id(Inpatient *head);
Inpatient* inpatient_find_by_id(Inpatient *head, int id);
Inpatient* inpatient_find_by_patient(Inpatient *head, const char *patient); // 返回第一个
int inpatient_update_discharge(Inpatient *ip, const char *discharge_date);
int inpatient_print_all(Inpatient *head);
int inpatient_free_all(Inpatient **head);

int inpatient_admit_auto(Inpatient **ihead, Billing **bhead, Ward *ward_head,
                         int inpatient_id, const char *patient_name, const char *ward_type,
                         float deposit, int planned_days, const char *admit_datetime);

/* ==================== 计费模块 ==================== */
Billing* billing_create(int id, int inpatient_id, const char *patient_name,
                        int ward_id, BillingType type, float amount,
                        const char *datetime, const char *note);
int billing_insert(Billing **head, Billing *node);
Billing* billing_find_by_id(Billing *head, int id);
int billing_print_all(Billing *head);
int billing_print_by_inpatient(Billing *head, int inpatient_id);
int billing_free_all(Billing **head);

int billing_validate_initial_deposit(float deposit, int planned_days);
float billing_daily_rate_by_ward(const Ward *w);

int billing_add_deposit(Billing **bhead, Inpatient *ip, int bill_id,
                        float amount, const char *datetime, const char *note);
int billing_topup(Billing **bhead, Inpatient *ip, int bill_id,
                  float amount, const char *datetime, const char *note);

int billing_auto_charge_until(Billing **bhead, Inpatient *ip, const Ward *w,
                              const char *now_datetime, int *charged_days, float *total_charged);
int billing_settle_on_discharge(Billing **bhead, Inpatient *ip, const Ward *w,
                                const char *discharge_datetime, float *total_cost, float *refund, float *due);
int billing_print_settlement(Billing *bhead, const Inpatient *ip, const Ward *w,
                             const char *as_of_datetime);

#endif // HIS_H