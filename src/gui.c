#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <wincodec.h>

#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "his.h"

/* Menu IDs */
#define IDM_OP1 1001
#define IDM_OP2 1002
#define IDM_OP3 1003
#define IDM_OP4 1004
#define IDM_OP5 1005
#define IDM_OP6 1006
#define IDM_OP7 1007
#define IDM_OP8 1008
#define IDM_OP9 1009

#define IDM_CLEAR_OUTPUT 2001
#define IDM_EXIT         3000

/* Controls */
#define IDC_OUTPUT 4001

static HWND g_main_hwnd = NULL;
static HWND g_output_edit = NULL;

static HBITMAP g_bg_bmp = NULL;
static int g_bg_w = 0;
static int g_bg_h = 0;

static HBITMAP g_bg_scaled_bmp = NULL;
static HBRUSH g_bg_scaled_brush = NULL;
static int g_bg_scaled_w = 0;
static int g_bg_scaled_h = 0;

static void bg_scaled_free(void) {
    if (g_bg_scaled_brush) {
        DeleteObject(g_bg_scaled_brush);
        g_bg_scaled_brush = NULL;
    }
    if (g_bg_scaled_bmp) {
        DeleteObject(g_bg_scaled_bmp);
        g_bg_scaled_bmp = NULL;
    }
    g_bg_scaled_w = g_bg_scaled_h = 0;
}

static void bg_scaled_ensure(int w, int h) {
    if (!g_bg_bmp) {
        bg_scaled_free();
        return;
    }
    if (w <= 0 || h <= 0) {
        bg_scaled_free();
        return;
    }
    if (g_bg_scaled_bmp && g_bg_scaled_brush && g_bg_scaled_w == w && g_bg_scaled_h == h) return;

    bg_scaled_free();

    HDC screen = GetDC(NULL);
    HDC src = CreateCompatibleDC(screen);
    HDC dst = CreateCompatibleDC(screen);
    HBITMAP scaled = CreateCompatibleBitmap(screen, w, h);
    if (!scaled) {
        DeleteDC(src);
        DeleteDC(dst);
        ReleaseDC(NULL, screen);
        return;
    }

    HGDIOBJ oldSrc = SelectObject(src, g_bg_bmp);
    HGDIOBJ oldDst = SelectObject(dst, scaled);
    SetStretchBltMode(dst, HALFTONE);
    StretchBlt(dst, 0, 0, w, h, src, 0, 0, g_bg_w, g_bg_h, SRCCOPY);
    SelectObject(src, oldSrc);
    SelectObject(dst, oldDst);

    DeleteDC(src);
    DeleteDC(dst);
    ReleaseDC(NULL, screen);

    g_bg_scaled_bmp = scaled;
    g_bg_scaled_brush = CreatePatternBrush(g_bg_scaled_bmp);
    g_bg_scaled_w = w;
    g_bg_scaled_h = h;
}

static wchar_t* utf8_to_wide_alloc(const char *s) {
    if (!s) return NULL;

    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    UINT cp = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (n == 0) {
        cp = CP_ACP;
        flags = 0;
        n = MultiByteToWideChar(cp, flags, s, -1, NULL, 0);
    }
    if (n <= 0) return NULL;

    wchar_t *w = (wchar_t*)calloc((size_t)n, sizeof(wchar_t));
    if (!w) return NULL;
    if (!MultiByteToWideChar(cp, flags, s, -1, w, n)) {
        free(w);
        return NULL;
    }
    return w;
}

static int wide_to_utf8(const wchar_t *w, char *out, size_t outlen) {
    if (!out || outlen == 0) return 0;
    out[0] = '\0';
    if (!w) return 0;
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, out, (int)outlen, NULL, NULL);
    if (n <= 0) return 0;
    return 1;
}

static void append_output(const char *text) {
    if (!g_output_edit || !text) return;

    /* Normalize line endings: Windows multiline EDIT expects CRLF. */
    size_t in_len = strlen(text);
    size_t extra = 0;
    for (size_t i = 0; i < in_len; i++) {
        if (text[i] == '\n' && (i == 0 || text[i - 1] != '\r')) extra++;
    }
    char *norm = NULL;
    if (extra) {
        norm = (char*)malloc(in_len + extra + 1);
        if (!norm) return;
        size_t j = 0;
        for (size_t i = 0; i < in_len; i++) {
            if (text[i] == '\n' && (i == 0 || text[i - 1] != '\r')) {
                norm[j++] = '\r';
                norm[j++] = '\n';
            } else {
                norm[j++] = text[i];
            }
        }
        norm[j] = '\0';
    }

    wchar_t *w = utf8_to_wide_alloc(norm ? norm : text);
    if (norm) free(norm);
    if (!w) return;
    int len = GetWindowTextLengthW(g_output_edit);
    SendMessageW(g_output_edit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(g_output_edit, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)w);
    SendMessageW(g_output_edit, EM_SCROLLCARET, 0, 0);
    free(w);
}

static void append_output_line(const char *text) {
    append_output(text);
    append_output("\r\n");
}

/* -------------------- Simple modal input box -------------------- */
typedef struct InputBoxState {
    const char *prompt;
    char *out;
    size_t outlen;
    int done;
    int ok;
    int multiline;
    HWND hEdit;
} InputBoxState;

static LRESULT CALLBACK inputbox_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    InputBoxState *st = (InputBoxState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW *cs = (CREATESTRUCTW*)lParam;
            st = (InputBoxState*)cs->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)st);

            HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            wchar_t *wprompt = utf8_to_wide_alloc(st->prompt ? st->prompt : "");
            CreateWindowExW(0, L"STATIC", wprompt ? wprompt : L"", WS_CHILD | WS_VISIBLE,
                            12, 12, 460, 36, hwnd, NULL, cs->hInstance, NULL);
            free(wprompt);

            DWORD editStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL;
            int editH = 24;
            if (st->multiline) {
                editStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL;
                editH = 80;
            }
            st->hEdit = CreateWindowExW(0, L"EDIT", L"", editStyle,
                                        12, 52, 460, editH, hwnd, (HMENU)1, cs->hInstance, NULL);

            HWND hOk = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       292, 52 + editH + 12, 80, 26, hwnd, (HMENU)IDOK, cs->hInstance, NULL);
            HWND hCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                           392, 52 + editH + 12, 80, 26, hwnd, (HMENU)IDCANCEL, cs->hInstance, NULL);

            SendMessageA(st->hEdit, WM_SETFONT, (WPARAM)font, TRUE);
            SendMessageA(hOk, WM_SETFONT, (WPARAM)font, TRUE);
            SendMessageA(hCancel, WM_SETFONT, (WPARAM)font, TRUE);

            SetFocus(st->hEdit);
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDOK) {
                if (st && st->out && st->outlen > 0 && st->hEdit) {
                    wchar_t wbuf[1024];
                    wbuf[0] = 0;
                    GetWindowTextW(st->hEdit, wbuf, (int)(sizeof(wbuf) / sizeof(wbuf[0])));
                    (void)wide_to_utf8(wbuf, st->out, st->outlen);
                }
                if (st) {
                    st->ok = 1;
                    st->done = 1;
                }
                DestroyWindow(hwnd);
                return 0;
            }
            if (id == IDCANCEL) {
                if (st) {
                    st->ok = 0;
                    st->done = 1;
                }
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            if (st) {
                st->ok = 0;
                st->done = 1;
            }
            DestroyWindow(hwnd);
            return 0;
        default:
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static int input_box(HWND parent, const char *title, const char *prompt, char *out, size_t outlen, int multiline) {
    if (!out || outlen == 0) return 0;
    out[0] = '\0';

    static int cls_inited = 0;
    HINSTANCE hinst = GetModuleHandle(NULL);
    if (!cls_inited) {
        WNDCLASSW wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = inputbox_wndproc;
        wc.hInstance = hinst;
        wc.lpszClassName = L"HISInputBox";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        if (!RegisterClassW(&wc)) {
            DWORD e = GetLastError();
            if (e != ERROR_CLASS_ALREADY_EXISTS) return 0;
        }
        cls_inited = 1;
    }

    InputBoxState st;
    ZeroMemory(&st, sizeof(st));
    st.prompt = prompt;
    st.out = out;
    st.outlen = outlen;
    st.multiline = multiline;

    EnableWindow(parent, FALSE);

    int w = 500;
    int h = multiline ? 190 : 140;

    RECT pr;
    GetWindowRect(parent, &pr);
    int x = pr.left + ((pr.right - pr.left) - w) / 2;
    int y = pr.top + ((pr.bottom - pr.top) - h) / 2;

    /* Clamp to current monitor work area to avoid off-screen dialog. */
    HMONITOR mon = MonitorFromWindow(parent, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    ZeroMemory(&mi, sizeof(mi));
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(mon, &mi)) {
        int minX = mi.rcWork.left;
        int minY = mi.rcWork.top;
        int maxX = mi.rcWork.right - w;
        int maxY = mi.rcWork.bottom - h;
        if (x < minX) x = minX;
        if (y < minY) y = minY;
        if (x > maxX) x = maxX;
        if (y > maxY) y = maxY;
    }

    wchar_t *wtitle = utf8_to_wide_alloc(title ? title : "Input");
    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME,
                               L"HISInputBox",
                               wtitle ? wtitle : L"Input",
                               WS_POPUP | WS_CAPTION | WS_SYSMENU,
                               x, y, w, h,
                               parent,
                               NULL,
                               hinst,
                               &st);
    free(wtitle);

    if (!dlg) {
        EnableWindow(parent, TRUE);
        SetActiveWindow(parent);
        return 0;
    }

    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    SetForegroundWindow(dlg);
    SetActiveWindow(dlg);

    MSG msg;
    while (!st.done && GetMessage(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessage(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);

    return st.ok;
}

static int input_int(HWND parent, const char *title, const char *prompt, int *out) {
    char buf[64];
    if (!input_box(parent, title, prompt, buf, sizeof(buf), 0)) return 0;
    char *end = NULL;
    long v = strtol(buf, &end, 10);
    if (end == buf) return 0;
    *out = (int)v;
    return 1;
}

static int input_float(HWND parent, const char *title, const char *prompt, float *out) {
    char buf[64];
    if (!input_box(parent, title, prompt, buf, sizeof(buf), 0)) return 0;
    char *end = NULL;
    double v = strtod(buf, &end);
    if (end == buf) return 0;
    *out = (float)v;
    return 1;
}

/* -------------------- stdout/stderr capture -------------------- */
typedef struct StdCapture {
    int old_out;
    int old_err;
    int read_fd;
    int active;
} StdCapture;

static int capture_begin(StdCapture *cap) {
    if (!cap) return 0;
    memset(cap, 0, sizeof(*cap));

    int pipefd[2];
    if (_pipe(pipefd, 64 * 1024, _O_BINARY) != 0) return 0;

    cap->old_out = _dup(_fileno(stdout));
    cap->old_err = _dup(_fileno(stderr));
    if (cap->old_out < 0 || cap->old_err < 0) {
        _close(pipefd[0]);
        _close(pipefd[1]);
        return 0;
    }

    (void)fflush(stdout);
    (void)fflush(stderr);

    if (_dup2(pipefd[1], _fileno(stdout)) != 0 || _dup2(pipefd[1], _fileno(stderr)) != 0) {
        _close(pipefd[0]);
        _close(pipefd[1]);
        _close(cap->old_out);
        _close(cap->old_err);
        return 0;
    }

    _close(pipefd[1]);
    cap->read_fd = pipefd[0];
    cap->active = 1;
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    return 1;
}

static char* capture_end(StdCapture *cap) {
    if (!cap || !cap->active) return NULL;

    (void)fflush(stdout);
    (void)fflush(stderr);

    (void)_dup2(cap->old_out, _fileno(stdout));
    (void)_dup2(cap->old_err, _fileno(stderr));
    _close(cap->old_out);
    _close(cap->old_err);

    cap->active = 0;

    size_t cap_size = 0;
    size_t cap_cap = 4096;
    char *buf = (char*)malloc(cap_cap);
    if (!buf) {
        _close(cap->read_fd);
        return NULL;
    }

    for (;;) {
        char tmp[2048];
        int n = _read(cap->read_fd, tmp, (unsigned)sizeof(tmp));
        if (n <= 0) break;
        if (cap_size + (size_t)n + 1 > cap_cap) {
            size_t new_cap = cap_cap;
            while (cap_size + (size_t)n + 1 > new_cap) new_cap *= 2;
            char *nb = (char*)realloc(buf, new_cap);
            if (!nb) break;
            buf = nb;
            cap_cap = new_cap;
        }
        memcpy(buf + cap_size, tmp, (size_t)n);
        cap_size += (size_t)n;
    }

    _close(cap->read_fd);
    buf[cap_size] = '\0';
    return buf;
}

static void run_and_show_output(void (*fn)(void)) {
    StdCapture cap;
    if (!capture_begin(&cap)) {
        append_output_line("[ERR] 无法捕获输出。\r\n");
        fn();
        return;
    }
    fn();
    char *out = capture_end(&cap);
    if (out) {
        append_output(out);
        free(out);
    }
}

/* -------------------- Helpers (ID generation) -------------------- */
static int next_patient_id_gui(void) {
    int max_id = 0;
    for (Patient *p = g_patient_head; p; p = p->next) if (p->id > max_id) max_id = p->id;
    return max_id + 1;
}
static int next_doctor_id_gui(void) {
    int max_id = 0;
    for (Doctor *d = g_doctor_head; d; d = d->next) if (d->id > max_id) max_id = d->id;
    return max_id + 1;
}
static int next_department_id_gui(void) {
    int max_id = 0;
    for (Department *d = g_department_head; d; d = d->next) if (d->id > max_id) max_id = d->id;
    return max_id + 1;
}
static int next_drug_id_gui(void) {
    int max_id = 0;
    for (Drug *d = g_drug_head; d; d = d->next) if (d->id > max_id) max_id = d->id;
    return max_id + 1;
}
static int next_ward_id_gui(void) {
    int max_id = 0;
    for (Ward *w = g_ward_head; w; w = w->next) if (w->id > max_id) max_id = w->id;
    return max_id + 1;
}
static int next_register_id_gui(void) {
    int max_id = 0;
    for (Register *r = g_register_head; r; r = r->next) if (r->id > max_id) max_id = r->id;
    return max_id + 1;
}
static int next_consultation_id_gui(void) {
    int max_id = 0;
    for (Consultation *c = g_consultation_head; c; c = c->next) if (c->id > max_id) max_id = c->id;
    return max_id + 1;
}
static int next_examination_id_gui(void) {
    int max_id = 0;
    for (Examination *e = g_examination_head; e; e = e->next) if (e->id > max_id) max_id = e->id;
    return max_id + 1;
}

static void op1_impl(void) {
    (void)patient_print_all(g_patient_head);
    (void)doctor_print_all(g_doctor_head);
    (void)department_print_all(g_department_head);
    (void)drug_print_all(g_drug_head);
    (void)ward_print_all(g_ward_head);
}

static void op2_impl(void) {
    (void)register_print_all(g_register_head);
    (void)consultation_print_all(g_consultation_head);
    (void)examination_print_all(g_examination_head);
    (void)inpatient_print_all(g_inpatient_head);
    (void)billing_print_all(g_billing_head);
}

static void do_op1(void) {
    append_output_line("\r\n===== 打印全部基础信息 =====");
    run_and_show_output(op1_impl);
}

static void do_op2(void) {
    append_output_line("\r\n===== 打印全部业务记录 =====");
    run_and_show_output(op2_impl);
}

static void do_op3_add_basic(HWND hwnd) {
    int sub = 0;
    append_output_line("\r\n===== 新增基础信息 =====");
    if (!input_int(hwnd, "新增基础信息", "选择：1患者 2医生 3科室 4药品 5病房", &sub)) {
        append_output_line("已取消");
        return;
    }

    if (sub == 1) {
        char name[50];
        int age = 0;
        int gender = 0;
        if (!input_box(hwnd, "新增患者", "姓名：", name, sizeof(name), 0)) return;
        if (!input_int(hwnd, "新增患者", "年龄：", &age)) return;
        if (!input_int(hwnd, "新增患者", "性别(0女1男)：", &gender)) return;
        int id = next_patient_id_gui();
        Patient *p = patient_create(id, name, age, gender ? 1 : 0);
        if (!p) {
            MessageBoxA(hwnd, "新增失败：内存不足", "HIS", MB_OK | MB_ICONERROR);
            return;
        }
        patient_insert(&g_patient_head, p);
        char msg[128];
        snprintf(msg, sizeof(msg), "新增患者成功：ID=%d", id);
        append_output_line(msg);
    } else if (sub == 2) {
        char name[50], dept[50], title[20], schedule[100];
        int age = 0;
        int gender = 0;
        if (!input_box(hwnd, "新增医生", "姓名：", name, sizeof(name), 0)) return;
        if (!input_int(hwnd, "新增医生", "年龄：", &age)) return;
        if (!input_int(hwnd, "新增医生", "性别(0女1男)：", &gender)) return;
        if (!input_box(hwnd, "新增医生", "所属科室：", dept, sizeof(dept), 0)) return;
        if (!input_box(hwnd, "新增医生", "职称：", title, sizeof(title), 0)) return;
        if (!input_box(hwnd, "新增医生", "出诊时间：", schedule, sizeof(schedule), 1)) return;
        int id = next_doctor_id_gui();
        Doctor *d = doctor_create(id, name, age, gender ? 1 : 0, dept, title, schedule);
        if (!d) {
            MessageBoxA(hwnd, "新增失败：内存不足", "HIS", MB_OK | MB_ICONERROR);
            return;
        }
        doctor_insert(&g_doctor_head, d);
        char msg[128];
        snprintf(msg, sizeof(msg), "新增医生成功：工号=%d", id);
        append_output_line(msg);
    } else if (sub == 3) {
        char name[50];
        if (!input_box(hwnd, "新增科室", "科室名称：", name, sizeof(name), 0)) return;
        int id = next_department_id_gui();
        Department *d = department_create(id, name);
        if (!d) {
            MessageBoxA(hwnd, "新增失败：内存不足", "HIS", MB_OK | MB_ICONERROR);
            return;
        }
        department_insert(&g_department_head, d);
        char msg[128];
        snprintf(msg, sizeof(msg), "新增科室成功：ID=%d", id);
        append_output_line(msg);
    } else if (sub == 4) {
        char name[50], alias[50], dept[50];
        int stock = 0;
        float price = 0.0f;
        if (!input_box(hwnd, "新增药品", "通用名：", name, sizeof(name), 0)) return;
        if (!input_box(hwnd, "新增药品", "别名：", alias, sizeof(alias), 0)) return;
        if (!input_int(hwnd, "新增药品", "库存：", &stock)) return;
        if (!input_float(hwnd, "新增药品", "单价：", &price)) return;
        if (!input_box(hwnd, "新增药品", "关联科室：", dept, sizeof(dept), 0)) return;
        int id = next_drug_id_gui();
        Drug *dr = drug_create(id, name, alias, stock, price, dept);
        if (!dr) {
            MessageBoxA(hwnd, "新增失败：内存不足", "HIS", MB_OK | MB_ICONERROR);
            return;
        }
        drug_insert(&g_drug_head, dr);
        char msg[128];
        snprintf(msg, sizeof(msg), "新增药品成功：ID=%d", id);
        append_output_line(msg);
    } else if (sub == 5) {
        char type[20], dept[50];
        int total_beds = 0;
        int can_convert = 0;
        if (!input_box(hwnd, "新增病房", "病房类型(Single/Double/Triple/ICU)：", type, sizeof(type), 0)) return;
        if (!input_int(hwnd, "新增病房", "总床位：", &total_beds)) return;
        if (!input_box(hwnd, "新增病房", "关联科室：", dept, sizeof(dept), 0)) return;
        if (!input_int(hwnd, "新增病房", "是否可转换(0否1是)：", &can_convert)) return;
        int id = next_ward_id_gui();
        Ward *w = ward_create(id, type, total_beds, dept, can_convert);
        if (!w) {
            MessageBoxA(hwnd, "新增失败：内存不足", "HIS", MB_OK | MB_ICONERROR);
            return;
        }
        ward_insert(&g_ward_head, w);
        char msg[128];
        snprintf(msg, sizeof(msg), "新增病房成功：ID=%d", id);
        append_output_line(msg);
    }
}

static void do_op4_add_business(HWND hwnd) {
    int sub = 0;
    append_output_line("\r\n===== 新增业务记录 =====");
    if (!input_int(hwnd, "新增业务记录", "选择：1挂号 2看诊 3检查 4办理住院(一键)", &sub)) {
        append_output_line("已取消");
        return;
    }

    if (sub == 1) {
        char patient[50], doctor[50], date[20];
        if (!input_box(hwnd, "新增挂号", "患者姓名：", patient, sizeof(patient), 0)) return;
        if (!input_box(hwnd, "新增挂号", "医生姓名：", doctor, sizeof(doctor), 0)) return;
        if (!input_box(hwnd, "新增挂号", "挂号日期(YYYY-MM-DD)：", date, sizeof(date), 0)) return;
        int id = next_register_id_gui();
        Register *r = register_create(id, patient, doctor, date);
        if (!r) {
            MessageBoxA(hwnd, "新增失败：内存不足", "HIS", MB_OK | MB_ICONERROR);
            return;
        }
        register_insert(&g_register_head, r);
        char msg[128];
        snprintf(msg, sizeof(msg), "新增挂号成功：ID=%d", id);
        append_output_line(msg);
    } else if (sub == 2) {
        char patient[50], doctor[50], date[20], diag[200], pres[200];
        if (!input_box(hwnd, "新增看诊", "患者姓名：", patient, sizeof(patient), 0)) return;
        if (!input_box(hwnd, "新增看诊", "医生姓名：", doctor, sizeof(doctor), 0)) return;
        if (!input_box(hwnd, "新增看诊", "看诊日期(YYYY-MM-DD)：", date, sizeof(date), 0)) return;
        if (!input_box(hwnd, "新增看诊", "诊断结果：", diag, sizeof(diag), 1)) return;
        if (!input_box(hwnd, "新增看诊", "处方信息：", pres, sizeof(pres), 1)) return;
        int id = next_consultation_id_gui();
        Consultation *c = consultation_create(id, patient, doctor, date, diag, pres);
        if (!c) {
            MessageBoxA(hwnd, "新增失败：内存不足", "HIS", MB_OK | MB_ICONERROR);
            return;
        }
        consultation_insert(&g_consultation_head, c);
        char msg[128];
        snprintf(msg, sizeof(msg), "新增看诊成功：ID=%d", id);
        append_output_line(msg);
    } else if (sub == 3) {
        char patient[50], item[50], date[20];
        float cost = 0.0f;
        if (!input_box(hwnd, "新增检查", "患者姓名：", patient, sizeof(patient), 0)) return;
        if (!input_box(hwnd, "新增检查", "检查项目：", item, sizeof(item), 0)) return;
        if (!input_float(hwnd, "新增检查", "检查费用：", &cost)) return;
        if (!input_box(hwnd, "新增检查", "检查日期(YYYY-MM-DD)：", date, sizeof(date), 0)) return;
        int id = next_examination_id_gui();
        Examination *e = examination_create(id, patient, item, cost, date);
        if (!e) {
            MessageBoxA(hwnd, "新增失败：内存不足", "HIS", MB_OK | MB_ICONERROR);
            return;
        }
        examination_insert(&g_examination_head, e);
        char msg[128];
        snprintf(msg, sizeof(msg), "新增检查成功：ID=%d", id);
        append_output_line(msg);
    } else if (sub == 4) {
        char patient[50], ward_type[20], admit_dt[24];
        float deposit = 0.0f;
        int planned_days = 0;
        if (!input_box(hwnd, "办理住院", "患者姓名：", patient, sizeof(patient), 0)) return;
        if (!input_box(hwnd, "办理住院", "病房类型(Single/Double/Triple/ICU)：", ward_type, sizeof(ward_type), 0)) return;
        if (!input_int(hwnd, "办理住院", "拟住院天数N：", &planned_days)) return;
        if (!input_float(hwnd, "办理住院", "押金金额：", &deposit)) return;
        if (!input_box(hwnd, "办理住院", "入院时间(YYYY-MM-DD 或 YYYY-MM-DDTHH:MM)：", admit_dt, sizeof(admit_dt), 0)) return;
        (void)inpatient_admit_auto(&g_inpatient_head, &g_billing_head, g_ward_head,
                                  0, patient, ward_type, deposit, planned_days, admit_dt);
        append_output_line("办理住院完成（如有失败信息请查看输出）。");
    }
}

static void do_op5_modify(HWND hwnd) {
    int sub = 0;
    append_output_line("\r\n===== 修改/删除 =====");
    if (!input_int(hwnd, "修改", "选择：1修改患者 2修改医生 3办理出院(结算) 4撤销+补录看诊 5删除记录", &sub)) {
        append_output_line("已取消");
        return;
    }

    if (sub == 1) {
        int id = 0;
        if (!input_int(hwnd, "修改患者", "患者ID：", &id)) return;
        Patient *p = patient_find_by_id(g_patient_head, id);
        if (!p) {
            MessageBoxA(hwnd, "未找到患者", "HIS", MB_OK | MB_ICONWARNING);
            return;
        }
        char name[50];
        int age = 0;
        int gender = 0;
        if (!input_box(hwnd, "修改患者", "新姓名：", name, sizeof(name), 0)) return;
        if (!input_int(hwnd, "修改患者", "新年龄：", &age)) return;
        if (!input_int(hwnd, "修改患者", "新性别(0女1男)：", &gender)) return;
        patient_update(p, name, age, gender ? 1 : 0);
        append_output_line("修改患者成功");
    } else if (sub == 2) {
        int id = 0;
        if (!input_int(hwnd, "修改医生", "医生工号：", &id)) return;
        Doctor *d = doctor_find_by_id(g_doctor_head, id);
        if (!d) {
            MessageBoxA(hwnd, "未找到医生", "HIS", MB_OK | MB_ICONWARNING);
            return;
        }
        char name[50], dept[50], title[20], schedule[100];
        int age = 0;
        int gender = 0;
        if (!input_box(hwnd, "修改医生", "新姓名：", name, sizeof(name), 0)) return;
        if (!input_int(hwnd, "修改医生", "新年龄：", &age)) return;
        if (!input_int(hwnd, "修改医生", "新性别(0女1男)：", &gender)) return;
        if (!input_box(hwnd, "修改医生", "新科室：", dept, sizeof(dept), 0)) return;
        if (!input_box(hwnd, "修改医生", "新职称：", title, sizeof(title), 0)) return;
        if (!input_box(hwnd, "修改医生", "新出诊时间：", schedule, sizeof(schedule), 1)) return;
        doctor_update(d, name, age, gender ? 1 : 0, dept, title, schedule);
        append_output_line("修改医生成功");
    } else if (sub == 3) {
        int inpatient_id = 0;
        char discharge_dt[24];
        if (!input_int(hwnd, "办理出院", "住院ID：", &inpatient_id)) return;
        Inpatient *ip = inpatient_find_by_id(g_inpatient_head, inpatient_id);
        if (!ip) {
            MessageBoxA(hwnd, "未找到住院记录", "HIS", MB_OK | MB_ICONWARNING);
            return;
        }
        if (!input_box(hwnd, "办理出院", "出院时间(YYYY-MM-DD 或 YYYY-MM-DDTHH:MM)：", discharge_dt, sizeof(discharge_dt), 0)) return;
        (void)inpatient_update_discharge(ip, discharge_dt);
        append_output_line("办理出院已提交（如有结算信息请查看输出）。");
    } else if (sub == 4) {
        int old_id = 0;
        char reason[80];
        if (!input_int(hwnd, "撤销看诊", "撤销的看诊记录ID：", &old_id)) return;
        if (!input_box(hwnd, "撤销看诊", "撤销原因：", reason, sizeof(reason), 0)) return;
        if (!consultation_revoke(g_consultation_head, old_id, reason)) {
            MessageBoxA(hwnd, "撤销失败：未找到记录或输入无效", "HIS", MB_OK | MB_ICONWARNING);
            return;
        }
        append_output_line("撤销完成，开始补录新记录。");
        char patient[50], doctor[50], date[20], diag[200], pres[200];
        if (!input_box(hwnd, "补录看诊", "患者姓名：", patient, sizeof(patient), 0)) return;
        if (!input_box(hwnd, "补录看诊", "医生姓名：", doctor, sizeof(doctor), 0)) return;
        if (!input_box(hwnd, "补录看诊", "看诊日期(YYYY-MM-DD)：", date, sizeof(date), 0)) return;
        if (!input_box(hwnd, "补录看诊", "诊断结果：", diag, sizeof(diag), 1)) return;
        if (!input_box(hwnd, "补录看诊", "处方信息：", pres, sizeof(pres), 1)) return;
        int new_id = next_consultation_id_gui();
        Consultation *c = consultation_create(new_id, patient, doctor, date, diag, pres);
        if (!c) {
            MessageBoxA(hwnd, "补录失败：内存不足", "HIS", MB_OK | MB_ICONERROR);
            return;
        }
        consultation_insert(&g_consultation_head, c);
        char msg[128];
        snprintf(msg, sizeof(msg), "补录成功：新看诊记录ID=%d", new_id);
        append_output_line(msg);
    } else if (sub == 5) {
        int del = 0;
        if (!input_int(hwnd, "删除记录", "选择：1患者 2医生 3科室 4药品 5病房 6挂号 7看诊 8检查 9住院", &del)) return;
        int id = 0;
        if (!input_int(hwnd, "删除记录", "ID：", &id)) return;
        int ok = 0;
        switch (del) {
            case 1: ok = patient_delete(&g_patient_head, id); break;
            case 2: ok = doctor_delete(&g_doctor_head, id); break;
            case 3: ok = department_delete(&g_department_head, id); break;
            case 4: ok = drug_delete(&g_drug_head, id); break;
            case 5: ok = ward_delete(&g_ward_head, id); break;
            case 6: ok = register_delete(&g_register_head, id); break;
            case 7: ok = consultation_delete(&g_consultation_head, id); break;
            case 8: ok = examination_delete(&g_examination_head, id); break;
            case 9: ok = inpatient_delete(&g_inpatient_head, id); break;
            default: ok = 0; break;
        }
        append_output_line(ok ? "删除成功" : "删除失败（未找到或条件不满足）");
    }
}

static void do_op6_query(HWND hwnd) {
    int sub = 0;
    append_output_line("\r\n===== 查询 =====");
    if (!input_int(hwnd, "查询", "选择：1患者ID 2医生ID 3科室ID 4药品ID 5病房ID 6挂号ID 7看诊ID 8检查ID 9住院ID 10计费ID 11患者名 12医生名 13科室名 14药品名 15患者维度历史", &sub)) {
        append_output_line("已取消");
        return;
    }

    if (sub >= 1 && sub <= 10) {
        int id = 0;
        if (!input_int(hwnd, "查询", "请输入ID：", &id)) return;
        char line[512];
        line[0] = '\0';
        if (sub == 1) {
            Patient *p = patient_find_by_id(g_patient_head, id);
            if (!p) strcpy(line, "未找到");
            else snprintf(line, sizeof(line), "ID=%d 姓名=%s 年龄=%d 性别=%s", p->id, p->name, p->age, p->gender ? "男" : "女");
        } else if (sub == 2) {
            Doctor *d = doctor_find_by_id(g_doctor_head, id);
            if (!d) strcpy(line, "未找到");
            else snprintf(line, sizeof(line), "工号=%d 姓名=%s 科室=%s 职称=%s 出诊=%s", d->id, d->name, d->department, d->title, d->schedule);
        } else if (sub == 3) {
            Department *dep = department_find_by_id(g_department_head, id);
            if (!dep) strcpy(line, "未找到");
            else snprintf(line, sizeof(line), "ID=%d 名称=%s", dep->id, dep->name);
        } else if (sub == 4) {
            Drug *dr = drug_find_by_id(g_drug_head, id);
            if (!dr) strcpy(line, "未找到");
            else snprintf(line, sizeof(line), "ID=%d 名称=%s 别名=%s 库存=%d 单价=%.2f 科室=%s", dr->id, dr->name, dr->alias, dr->stock, dr->price, dr->department);
        } else if (sub == 5) {
            Ward *w = ward_find_by_id(g_ward_head, id);
            if (!w) strcpy(line, "未找到");
            else snprintf(line, sizeof(line), "ID=%d 类型=%s 总床位=%d 已占=%d 科室=%s 可转换=%s", w->id, w->type, w->total_beds, w->occupied_beds, w->department, w->can_convert ? "是" : "否");
        } else if (sub == 6) {
            Register *r = register_find_by_id(g_register_head, id);
            if (!r) strcpy(line, "未找到");
            else snprintf(line, sizeof(line), "ID=%d 患者=%s 医生=%s 日期=%s", r->id, r->patient_name, r->doctor_name, r->date);
        } else if (sub == 7) {
            Consultation *c = consultation_find_by_id(g_consultation_head, id);
            if (!c) strcpy(line, "未找到");
            else snprintf(line, sizeof(line),
                          "ID=%d 患者=%.50s 医生=%.50s 日期=%.20s 诊断=%.160s 处方=%.160s",
                          c->id, c->patient_name, c->doctor_name, c->date, c->diagnosis, c->prescription);
        } else if (sub == 8) {
            Examination *e = examination_find_by_id(g_examination_head, id);
            if (!e) strcpy(line, "未找到");
            else snprintf(line, sizeof(line), "ID=%d 患者=%s 项目=%s 费用=%.2f 日期=%s", e->id, e->patient_name, e->item, e->cost, e->date);
        } else if (sub == 9) {
            Inpatient *ip = inpatient_find_by_id(g_inpatient_head, id);
            if (!ip) strcpy(line, "未找到");
            else snprintf(line, sizeof(line), "住院ID=%d 患者=%s 病房ID=%d 床位=%d 押金余额=%.2f 入院=%s 出院=%s",
                         ip->id, ip->patient_name, ip->ward_id, ip->bed_num, ip->deposit, ip->admit_date,
                         ip->discharge_date[0] ? ip->discharge_date : "未出院");
        } else if (sub == 10) {
            Billing *b = billing_find_by_id(g_billing_head, id);
            if (!b) strcpy(line, "未找到");
            else snprintf(line, sizeof(line), "ID=%d 住院ID=%d 患者=%s 病房ID=%d 金额=%.2f 时间=%s 备注=%s",
                         b->id, b->inpatient_id, b->patient_name, b->ward_id, b->amount, b->datetime, b->note[0] ? b->note : "-");
        }
        append_output_line(line);
        return;
    }

    if (sub >= 11 && sub <= 14) {
        char name[50];
        if (!input_box(hwnd, "查询", "请输入名称：", name, sizeof(name), 0)) return;
        char line[512];
        line[0] = '\0';
        if (sub == 11) {
            Patient *p = patient_find_by_name(g_patient_head, name);
            if (!p) strcpy(line, "未找到或重名（请用ID）");
            else snprintf(line, sizeof(line), "ID=%d 姓名=%s 年龄=%d 性别=%s", p->id, p->name, p->age, p->gender ? "男" : "女");
        } else if (sub == 12) {
            Doctor *d = doctor_find_by_name(g_doctor_head, name);
            if (!d) strcpy(line, "未找到或重名（请用工号）");
            else snprintf(line, sizeof(line), "工号=%d 姓名=%s 科室=%s 职称=%s", d->id, d->name, d->department, d->title);
        } else if (sub == 13) {
            Department *dep = department_find_by_name(g_department_head, name);
            if (!dep) strcpy(line, "未找到或重名（请用ID）");
            else snprintf(line, sizeof(line), "ID=%d 名称=%s", dep->id, dep->name);
        } else if (sub == 14) {
            Drug *dr = drug_find_by_name(g_drug_head, name);
            if (!dr) strcpy(line, "未找到或重名（请用ID）");
            else snprintf(line, sizeof(line), "ID=%d 名称=%s 别名=%s 库存=%d 单价=%.2f 科室=%s", dr->id, dr->name, dr->alias, dr->stock, dr->price, dr->department);
        }
        append_output_line(line);
        return;
    }

    if (sub == 15) {
        char name[50];
        if (!input_box(hwnd, "患者维度历史", "患者姓名：", name, sizeof(name), 0)) return;
        append_output_line("\r\n===== 患者维度历史 =====");
        StdCapture cap;
        if (!capture_begin(&cap)) return;
        (void)his_print_patient_history(name);
        char *out = capture_end(&cap);
        if (out) {
            append_output(out);
            free(out);
        }
    }
}

static void do_op7_summary(HWND hwnd) {
    int sub = 0;
    append_output_line("\r\n===== 汇总统计 =====");
    if (!input_int(hwnd, "汇总统计", "选择：1医生维度(工号) 2科室维度 3时间范围全院", &sub)) {
        append_output_line("已取消");
        return;
    }

    if (sub == 1) {
        int id = 0;
        char date[20];
        if (!input_int(hwnd, "医生维度", "医生工号：", &id)) return;
        if (!input_box(hwnd, "医生维度", "日期(YYYY-MM-DD，留空统计全部)：", date, sizeof(date), 0)) return;

        StdCapture cap;
        if (!capture_begin(&cap)) return;
        (void)his_summary_by_doctor_id(id, date[0] ? date : NULL);
        char *out = capture_end(&cap);
        if (out) {
            append_output(out);
            free(out);
        }
    } else if (sub == 2) {
        char dept[50], start[20], end[20];
        if (!input_box(hwnd, "科室维度", "科室名称：", dept, sizeof(dept), 0)) return;
        if (!input_box(hwnd, "科室维度", "开始日期(YYYY-MM-DD，留空不限制)：", start, sizeof(start), 0)) return;
        if (!input_box(hwnd, "科室维度", "结束日期(YYYY-MM-DD，留空不限制)：", end, sizeof(end), 0)) return;

        StdCapture cap;
        if (!capture_begin(&cap)) return;
        (void)his_summary_by_department(dept, start[0] ? start : NULL, end[0] ? end : NULL);
        char *out = capture_end(&cap);
        if (out) {
            append_output(out);
            free(out);
        }
    } else if (sub == 3) {
        char start[20], end[20];
        if (!input_box(hwnd, "时间范围全院", "开始日期(YYYY-MM-DD)：", start, sizeof(start), 0)) return;
        if (!input_box(hwnd, "时间范围全院", "结束日期(YYYY-MM-DD)：", end, sizeof(end), 0)) return;

        StdCapture cap;
        if (!capture_begin(&cap)) return;
        (void)his_summary_by_date_range(start, end);
        char *out = capture_end(&cap);
        if (out) {
            append_output(out);
            free(out);
        }
    }
}

static void do_op8_billing(HWND hwnd) {
    int sub = 0;
    append_output_line("\r\n===== 住院计费操作 =====");
    if (!input_int(hwnd, "住院计费操作", "选择：1补扣到时间点 2补缴押金 3结算预览 4打印计费流水", &sub)) {
        append_output_line("已取消");
        return;
    }

    if (sub == 1) {
        int inpatient_id = 0;
        char now_dt[24];
        if (!input_int(hwnd, "补扣", "住院ID：", &inpatient_id)) return;
        Inpatient *ip = inpatient_find_by_id(g_inpatient_head, inpatient_id);
        if (!ip) { MessageBoxA(hwnd, "未找到住院记录", "HIS", MB_OK | MB_ICONWARNING); return; }
        Ward *w = ward_find_by_id(g_ward_head, ip->ward_id);
        if (!w) { MessageBoxA(hwnd, "未找到病房", "HIS", MB_OK | MB_ICONWARNING); return; }
        if (!input_box(hwnd, "补扣", "截至时间(YYYY-MM-DD 或 YYYY-MM-DDTHH:MM)：", now_dt, sizeof(now_dt), 0)) return;
        int days = 0;
        float total = 0.0f;
        if (billing_auto_charge_until(&g_billing_head, ip, w, now_dt, &days, &total)) {
            char line[256];
            snprintf(line, sizeof(line), "补扣完成：新增扣费天数=%d 新增费用=%.2f 当前押金余额=%.2f", days, total, ip->deposit);
            append_output_line(line);
        } else {
            append_output_line("补扣失败");
        }
    } else if (sub == 2) {
        int inpatient_id = 0;
        char dt[24];
        float amount = 0.0f;
        if (!input_int(hwnd, "补缴", "住院ID：", &inpatient_id)) return;
        Inpatient *ip = inpatient_find_by_id(g_inpatient_head, inpatient_id);
        if (!ip) { MessageBoxA(hwnd, "未找到住院记录", "HIS", MB_OK | MB_ICONWARNING); return; }
        if (!input_float(hwnd, "补缴", "补缴金额：", &amount)) return;
        if (!input_box(hwnd, "补缴", "时间(YYYY-MM-DD 或 YYYY-MM-DDTHH:MM)：", dt, sizeof(dt), 0)) return;
        if (billing_topup(&g_billing_head, ip, 0, amount, dt, "manual_topup")) {
            char line[128];
            snprintf(line, sizeof(line), "补缴成功：当前押金余额=%.2f", ip->deposit);
            append_output_line(line);
        } else {
            append_output_line("补缴失败");
        }
    } else if (sub == 3) {
        int inpatient_id = 0;
        char asof[24];
        if (!input_int(hwnd, "结算预览", "住院ID：", &inpatient_id)) return;
        Inpatient *ip = inpatient_find_by_id(g_inpatient_head, inpatient_id);
        if (!ip) { MessageBoxA(hwnd, "未找到住院记录", "HIS", MB_OK | MB_ICONWARNING); return; }
        Ward *w = ward_find_by_id(g_ward_head, ip->ward_id);
        if (!w) { MessageBoxA(hwnd, "未找到病房", "HIS", MB_OK | MB_ICONWARNING); return; }
        if (!input_box(hwnd, "结算预览", "截至时间(YYYY-MM-DD 或 YYYY-MM-DDTHH:MM)：", asof, sizeof(asof), 0)) return;

        append_output_line("\r\n===== 结算预览 =====");
        StdCapture cap;
        if (!capture_begin(&cap)) return;
        (void)billing_print_settlement(g_billing_head, ip, w, asof);
        char *out = capture_end(&cap);
        if (out) {
            append_output(out);
            free(out);
        }
    } else if (sub == 4) {
        int which = 0;
        if (!input_int(hwnd, "打印计费流水", "选择：1全部流水 2按住院ID", &which)) return;
        append_output_line("\r\n===== 计费流水 =====");
        StdCapture cap;
        if (!capture_begin(&cap)) return;
        if (which == 1) {
            (void)billing_print_all(g_billing_head);
        } else if (which == 2) {
            int inpatient_id = 0;
            if (!input_int(hwnd, "打印计费流水", "住院ID：", &inpatient_id)) { free(capture_end(&cap)); return; }
            (void)billing_print_by_inpatient(g_billing_head, inpatient_id);
        }
        char *out = capture_end(&cap);
        if (out) {
            append_output(out);
            free(out);
        }
    }
}

static void do_op9_save(HWND hwnd) {
    (void)hwnd;
    if (!his_save("data")) {
        append_output_line("保存失败");
    } else {
        append_output_line("保存完成");
    }
}

/* -------------------- Background image (WIC, pure C) -------------------- */
static HBITMAP load_bitmap_wic(const wchar_t *path, int *out_w, int *out_h) {
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    if (!path) return NULL;

    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    IWICFormatConverter *conv = NULL;

    HRESULT hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                                  &IID_IWICImagingFactory, (LPVOID*)&factory);
    if (FAILED(hr) || !factory) goto cleanup;

    hr = factory->lpVtbl->CreateDecoderFromFilename(factory, path, NULL, GENERIC_READ,
                                                    WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr) || !decoder) goto cleanup;

    hr = decoder->lpVtbl->GetFrame(decoder, 0, &frame);
    if (FAILED(hr) || !frame) goto cleanup;

    hr = factory->lpVtbl->CreateFormatConverter(factory, &conv);
    if (FAILED(hr) || !conv) goto cleanup;

    hr = conv->lpVtbl->Initialize(conv, (IWICBitmapSource*)frame,
                                  &GUID_WICPixelFormat32bppPBGRA,
                                  WICBitmapDitherTypeNone, NULL, 0.0,
                                  WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) goto cleanup;

    UINT w = 0, h = 0;
    hr = conv->lpVtbl->GetSize(conv, &w, &h);
    if (FAILED(hr) || w == 0 || h == 0) goto cleanup;

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (LONG)w;
    bmi.bmiHeader.biHeight = -(LONG)h; /* top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *bits = NULL;
    HDC hdc = GetDC(NULL);
    HBITMAP hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(NULL, hdc);
    if (!hbmp || !bits) goto cleanup;

    const UINT stride = w * 4;
    const UINT bufSize = stride * h;
    hr = conv->lpVtbl->CopyPixels(conv, NULL, stride, bufSize, (BYTE*)bits);
    if (FAILED(hr)) {
        DeleteObject(hbmp);
        hbmp = NULL;
        goto cleanup;
    }

    if (out_w) *out_w = (int)w;
    if (out_h) *out_h = (int)h;

cleanup:
    if (conv) conv->lpVtbl->Release(conv);
    if (frame) frame->lpVtbl->Release(frame);
    if (decoder) decoder->lpVtbl->Release(decoder);
    if (factory) factory->lpVtbl->Release(factory);
    return hbmp;
}

static void load_background_image(void) {
    if (g_bg_bmp) return;

    wchar_t exePath[MAX_PATH];
    exePath[0] = 0;
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t *slash = wcsrchr(exePath, L'\\');
    if (slash) *(slash + 1) = L'\0';

    const wchar_t *candidates[] = {
        L"assets\\background.png",
        L"assets\\background.jpg",
        L"assets\\background.jpeg",
    };

    wchar_t full[MAX_PATH];
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        _snwprintf(full, MAX_PATH, L"%ls%ls", exePath, candidates[i]);
        full[MAX_PATH - 1] = 0;
        g_bg_bmp = load_bitmap_wic(full, &g_bg_w, &g_bg_h);
        if (g_bg_bmp) break;
    }
}

static void paint_background(HWND hwnd, HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    bg_scaled_ensure(rc.right - rc.left, rc.bottom - rc.top);

    if (g_bg_bmp) {
        HDC mem = CreateCompatibleDC(hdc);
        HGDIOBJ old = SelectObject(mem, g_bg_bmp);
        SetStretchBltMode(hdc, HALFTONE);
        StretchBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                   mem, 0, 0, g_bg_w, g_bg_h, SRCCOPY);
        SelectObject(mem, old);
        DeleteDC(mem);
        return;
    }

    HBRUSH br = CreateSolidBrush(RGB(245, 245, 245));
    FillRect(hdc, &rc, br);
    DeleteObject(br);
}

/* -------------------- Window proc -------------------- */
static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW *cs = (CREATESTRUCTW*)lParam;
            HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            g_output_edit = CreateWindowExW(
                0,
                L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                12,
                12,
                600,
                360,
                hwnd,
                (HMENU)IDC_OUTPUT,
                cs->hInstance,
                NULL);
            SendMessageW(g_output_edit, WM_SETFONT, (WPARAM)font, TRUE);
            append_output_line("HIS GUI 已启动。");
            return 0;
        }
        case WM_SIZE: {
            if (g_output_edit) {
                int w = LOWORD(lParam);
                int h = HIWORD(lParam);
                int margin = 12;
                MoveWindow(g_output_edit, margin, margin, w - margin * 2, h - margin * 2, TRUE);
            }
            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                bg_scaled_ensure(rc.right - rc.left, rc.bottom - rc.top);
            }
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            switch (id) {
                case IDM_OP1: do_op1(); return 0;
                case IDM_OP2: do_op2(); return 0;
                case IDM_OP3: do_op3_add_basic(hwnd); return 0;
                case IDM_OP4: do_op4_add_business(hwnd); return 0;
                case IDM_OP5: do_op5_modify(hwnd); return 0;
                case IDM_OP6: do_op6_query(hwnd); return 0;
                case IDM_OP7: do_op7_summary(hwnd); return 0;
                case IDM_OP8: do_op8_billing(hwnd); return 0;
                case IDM_OP9: do_op9_save(hwnd); return 0;
                case IDM_CLEAR_OUTPUT:
                    if (g_output_edit) SetWindowTextW(g_output_edit, L"");
                    return 0;
                case IDM_EXIT:
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;
                default:
                    break;
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            paint_background(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            HWND ctl = (HWND)lParam;
            if (ctl == g_output_edit) {
                SetTextColor(hdc, RGB(0, 0, 0));
                /* Must be OPAQUE for EDIT to avoid repaint artifacts. */
                SetBkMode(hdc, OPAQUE);

                if (g_bg_scaled_brush) {
                    POINT pt = {0, 0};
                    MapWindowPoints(ctl, hwnd, &pt, 1);
                    SetBrushOrgEx(hdc, -pt.x, -pt.y, NULL);
                    return (INT_PTR)g_bg_scaled_brush;
                }
                return (INT_PTR)(HBRUSH)(COLOR_WINDOW + 1);
            }
            break;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND ctl = (HWND)lParam;
            if (ctl == g_output_edit) {
                SetTextColor(hdc, RGB(0, 0, 0));
                /* Must be OPAQUE for read-only EDIT (often routes here). */
                SetBkMode(hdc, OPAQUE);

                if (g_bg_scaled_brush) {
                    POINT pt = {0, 0};
                    MapWindowPoints(ctl, hwnd, &pt, 1);
                    SetBrushOrgEx(hdc, -pt.x, -pt.y, NULL);
                    return (INT_PTR)g_bg_scaled_brush;
                }
                return (INT_PTR)(HBRUSH)(COLOR_WINDOW + 1);
            }
            break;
        }
        case WM_ERASEBKGND:
            /* We paint the background in WM_PAINT */
            return 1;
        case WM_CLOSE: {
            int ans = MessageBoxW(hwnd, L"退出前是否保存数据？", L"HIS", MB_YESNOCANCEL | MB_ICONQUESTION);
            if (ans == IDCANCEL) return 0;
            if (ans == IDYES) {
                if (!his_save("data")) {
                    MessageBoxW(hwnd, L"保存失败", L"HIS", MB_OK | MB_ICONERROR);
                }
            }
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_DESTROY:
            bg_scaled_free();
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static HMENU create_main_menu(void) {
    HMENU menu_bar = CreateMenu();
    HMENU menu_ops = CreateMenu();

    AppendMenuW(menu_ops, MF_STRING, IDM_OP1, L"1. 打印全部基础信息");
    AppendMenuW(menu_ops, MF_STRING, IDM_OP2, L"2. 打印全部业务记录");
    AppendMenuW(menu_ops, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu_ops, MF_STRING, IDM_OP3, L"3. 新增基础信息");
    AppendMenuW(menu_ops, MF_STRING, IDM_OP4, L"4. 新增业务记录");
    AppendMenuW(menu_ops, MF_STRING, IDM_OP5, L"5. 修改/删除");
    AppendMenuW(menu_ops, MF_STRING, IDM_OP6, L"6. 查询");
    AppendMenuW(menu_ops, MF_STRING, IDM_OP7, L"7. 汇总统计");
    AppendMenuW(menu_ops, MF_STRING, IDM_OP8, L"8. 住院计费操作");
    AppendMenuW(menu_ops, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu_ops, MF_STRING, IDM_OP9, L"9. 保存数据到 data/");

    AppendMenuW(menu_bar, MF_POPUP, (UINT_PTR)menu_ops, L"功能");
    AppendMenuW(menu_bar, MF_STRING, IDM_CLEAR_OUTPUT, L"清空输出");
    AppendMenuW(menu_bar, MF_STRING, IDM_EXIT, L"退出");
    return menu_bar;
}

int his_gui_main(void) {
    HINSTANCE hinst = GetModuleHandle(NULL);

    /* In -mwindows subsystem apps, stdout/stderr may not be usable.
       Redirect them to NUL so we can safely dup/redirect for capture. */
    (void)freopen("NUL", "w", stdout);
    (void)freopen("NUL", "w", stderr);
    (void)freopen("NUL", "r", stdin);

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    (void)hr;
    load_background_image();

    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = hinst;
    wc.lpszClassName = L"HISMainWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"RegisterClass failed.", L"HIS", MB_OK | MB_ICONERROR);
        if (g_bg_bmp) {
            DeleteObject(g_bg_bmp);
            g_bg_bmp = NULL;
            g_bg_w = g_bg_h = 0;
        }
        CoUninitialize();
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Hospital Information System (GUI)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900,
        600,
        NULL,
        NULL,
        hinst,
        NULL);

    if (!hwnd) {
        MessageBoxW(NULL, L"CreateWindow failed.", L"HIS", MB_OK | MB_ICONERROR);
        if (g_bg_bmp) {
            DeleteObject(g_bg_bmp);
            g_bg_bmp = NULL;
            g_bg_w = g_bg_h = 0;
        }
        CoUninitialize();
        return 1;
    }

    g_main_hwnd = hwnd;
    SetMenu(hwnd, create_main_menu());

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_bg_bmp) {
        DeleteObject(g_bg_bmp);
        g_bg_bmp = NULL;
        g_bg_w = g_bg_h = 0;
    }
    bg_scaled_free();
    CoUninitialize();

    return (int)msg.wParam;
}
