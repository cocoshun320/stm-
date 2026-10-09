// STM32 开发环境一键安装程序
// 功能:
//   1. 检测是否已安装(多种方式)，已安装则显示"已安装，无需安装"并直接跳过
//   2. 未安装则释放并运行安装包，安装完立即删除临时文件
//   3. 检测安装程序是否正常关闭（意外终止则报告失败）
//   4. 已安装直接跳过，不询问 Y/N
//   程序名: stm install
// 编译: g++ -O2 -o "stm install.exe" installer.cpp resource.o -static

#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>

// 嵌入资源的资源ID
#define RES_CH340        100
#define RES_JRE          101
#define RES_STM32CUBE    102

#define NUM_COMPONENTS 3

// 组件信息结构
struct Component {
    int resId;
    const char* fileName;
    const char* displayName;
};

static Component g_components[NUM_COMPONENTS] = {
    { RES_CH340,     "CH340SETUP.EXE",                                          "CH340 串口驱动" },
    { RES_JRE,       "jre-8u503-windows-x64.exe",                               "Java 运行环境 (JRE 8)" },
    { RES_STM32CUBE, "st-stm32cubeide_1.17.0_23558_20241125_2245_x86_64.exe",  "STM32CubeIDE 1.17.0" }
};

// ============================================================
// 工具函数
// ============================================================

// 检查文件是否存在
static bool FileExists(const char* path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

// 检查目录是否存在
static bool DirExists(const char* path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

// 检查注册表键是否存在
static bool RegKeyExists(HKEY root, const char* path) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(root, path, 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

// 读取注册表字符串值
static std::string RegReadString(HKEY root, const char* path, const char* valueName) {
    HKEY hKey;
    if (RegOpenKeyExA(root, path, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return "";
    char buf[1024] = {0};
    DWORD bufSize = sizeof(buf);
    DWORD type = 0;
    LONG result = RegQueryValueExA(hKey, valueName, NULL, &type, (LPBYTE)buf, &bufSize);
    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS) return "";
    return std::string(buf);
}

// 在 Uninstall 注册表项中搜索包含关键字的 DisplayName
static bool FindInUninstallEntries(const char* keyword, std::string& outName) {
    const char* paths[] = {
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
    };

    for (int p = 0; p < 2; p++) {
        HKEY hUninstall;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, paths[p], 0, KEY_READ, &hUninstall) != ERROR_SUCCESS)
            continue;

        DWORD index = 0;
        char subKeyName[256];
        DWORD subKeySize;

        while (true) {
            subKeySize = sizeof(subKeyName);
            LONG result = RegEnumKeyExA(hUninstall, index, subKeyName, &subKeySize,
                                         NULL, NULL, NULL, NULL);
            if (result != ERROR_SUCCESS) break;
            index++;

            std::string fullKey = std::string(paths[p]) + "\\" + subKeyName;
            std::string name = RegReadString(HKEY_LOCAL_MACHINE, fullKey.c_str(), "DisplayName");
            if (name.empty()) continue;

            std::string nameLower = name;
            std::string keywordLower = keyword;
            for (size_t i = 0; i < nameLower.size(); i++)
                nameLower[i] = tolower((unsigned char)nameLower[i]);
            for (size_t i = 0; i < keywordLower.size(); i++)
                keywordLower[i] = tolower((unsigned char)keywordLower[i]);

            if (nameLower.find(keywordLower) != std::string::npos) {
                outName = name;
                RegCloseKey(hUninstall);
                return true;
            }
        }
        RegCloseKey(hUninstall);
    }
    return false;
}

// ============================================================
// 组件已安装检测（每种组件用不同方式）
// ============================================================

// 检测 CH340 是否已安装
// 方式: 1.驱动文件存在 2.注册表服务 3.Uninstall项
static bool IsCH340Installed(std::string& detail) {
    // 方式1: 检测驱动文件 (CH341SER.SYS / CH341S98.SYS)
    if (FileExists("C:\\Windows\\System32\\drivers\\CH341SER.SYS")) {
        detail = "驱动文件 CH341SER.SYS 已存在";
        return true;
    }
    if (FileExists("C:\\Windows\\System32\\drivers\\CH341S98.SYS")) {
        detail = "驱动文件 CH341S98.SYS 已存在";
        return true;
    }

    // 方式2: 注册表服务项
    const char* services[] = {
        "SYSTEM\\CurrentControlSet\\services\\ch343",
        "SYSTEM\\CurrentControlSet\\services\\ch340",
        "SYSTEM\\CurrentControlSet\\services\\ch341",
    };
    for (size_t i = 0; i < sizeof(services)/sizeof(services[0]); i++) {
        if (RegKeyExists(HKEY_LOCAL_MACHINE, services[i])) {
            detail = "驱动服务已注册";
            return true;
        }
    }

    // 方式3: Uninstall 项搜索
    std::string name;
    if (FindInUninstallEntries("CH340", name) || FindInUninstallEntries("ch341", name)) {
        detail = "已在已安装程序列表中找到: " + name;
        return true;
    }

    return false;
}

// 检测 JRE 8 是否已安装
// 方式: 1.注册表JavaSoft 2.常见安装路径 3.Uninstall项 4.PATH中的java
static bool IsJRE8Installed(std::string& detail) {
    // 方式1: 注册表 JavaSoft\Java Runtime Environment\1.8
    if (RegKeyExists(HKEY_LOCAL_MACHINE, "SOFTWARE\\JavaSoft\\Java Runtime Environment\\1.8")) {
        std::string ver = RegReadString(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\JavaSoft\\Java Runtime Environment\\1.8", "RuntimeLib");
        detail = "注册表: JavaSoft\\Java Runtime Environment\\1.8";
        return true;
    }
    if (RegKeyExists(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\JavaSoft\\Java Runtime Environment\\1.8")) {
        detail = "注册表: WOW6432Node\\JavaSoft\\Java Runtime Environment\\1.8";
        return true;
    }

    // 方式2: 常见安装路径下的 java.exe
    const char* paths[] = {
        "C:\\Program Files\\Java\\jre-1.8\\bin\\java.exe",
        "C:\\Program Files\\Java\\jre8\\bin\\java.exe",
        "C:\\Program Files\\Java\\jre-8u503-windows-x64\\bin\\java.exe",
        "C:\\Program Files (x86)\\Java\\jre-1.8\\bin\\java.exe",
        "C:\\Program Files (x86)\\Java\\jre8\\bin\\java.exe",
        "C:\\Program Files\\Common Files\\Oracle\\Java\\javapath\\java.exe",
    };
    for (size_t i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
        if (FileExists(paths[i])) {
            detail = std::string("已安装: ") + paths[i];
            return true;
        }
    }

    // 方式3: Uninstall 项搜索
    std::string name;
    if (FindInUninstallEntries("Java 8", name) ||
        FindInUninstallEntries("Java(TM)", name) ||
        FindInUninstallEntries("Java 7", name)) {  // 有些版本号显示不同
        detail = "已安装: " + name;
        return true;
    }

    // 方式4: 搜索 PATH 中的 java.exe
    char pathEnv[32768] = {0};
    GetEnvironmentVariableA("PATH", pathEnv, sizeof(pathEnv));
    std::string pathStr = pathEnv;
    size_t pos = 0;
    while (pos < pathStr.size()) {
        size_t end = pathStr.find(';', pos);
        if (end == std::string::npos) end = pathStr.size();
        std::string dir = pathStr.substr(pos, end - pos);
        std::string javaPath = dir + "\\java.exe";
        if (FileExists(javaPath.c_str())) {
            detail = "PATH 中找到: " + javaPath;
            return true;
        }
        pos = end + 1;
    }

    return false;
}

// 检测 STM32CubeIDE 是否已安装
// 方式: 1.安装目录 2.Uninstall项 3.可执行文件
static bool IsSTM32CubeIDEInstalled(std::string& detail) {
    // 方式1: 安装目录
    if (DirExists("C:\\ST\\STM32CubeIDE_1.17.0")) {
        detail = "安装目录: C:\\ST\\STM32CubeIDE_1.17.0";
        return true;
    }

    // 方式2: 可执行文件
    if (FileExists("C:\\ST\\STM32CubeIDE_1.17.0\\STM32CubeIDE\\stm32cubeide.exe")) {
        detail = "已安装: stm32cubeide.exe";
        return true;
    }

    // 方式3: Uninstall 项搜索
    std::string name;
    if (FindInUninstallEntries("STM32CubeIDE", name)) {
        detail = "已安装: " + name;
        return true;
    }

    // 方式4: 搜索 STMicroelectronics 相关的 STM32 项
    if (FindInUninstallEntries("STM32Cube", name)) {
        detail = "已安装: " + name;
        return true;
    }

    return false;
}

// 统一检测接口
static bool IsComponentInstalled(int index, std::string& detail) {
    switch (g_components[index].resId) {
        case RES_CH340:     return IsCH340Installed(detail);
        case RES_JRE:       return IsJRE8Installed(detail);
        case RES_STM32CUBE: return IsSTM32CubeIDEInstalled(detail);
    }
    return false;
}

// ============================================================
// 文件操作
// ============================================================

static bool ExtractResource(int resId, const std::string& outPath) {
    HRSRC hRes = FindResourceA(NULL, MAKEINTRESOURCEA(resId), "INSTALLER");
    if (!hRes) return false;
    HGLOBAL hMem = LoadResource(NULL, hRes);
    if (!hMem) return false;
    DWORD size = SizeofResource(NULL, hRes);
    void* pData = LockResource(hMem);
    if (!pData || size == 0) return false;

    HANDLE hFile = CreateFileA(outPath.c_str(), GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, pData, size, &written, NULL);
    CloseHandle(hFile);
    return (ok && written == size);
}

static DWORD RunAndWait(const std::string& cmd) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::string commandLine = cmd;
    commandLine.resize(commandLine.size() + 1);

    BOOL ok = CreateProcessA(NULL, const_cast<LPSTR>(commandLine.c_str()),
                             NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (!ok) return (DWORD)-1;

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode;
}

static std::string GetTempDir() {
    char buf[MAX_PATH + 1] = {0};
    GetTempPathA(MAX_PATH, buf);
    return std::string(buf);
}

// ============================================================
// 安装逻辑
// ============================================================

enum InstallResult {
    RESULT_SUCCESS = 0,
    RESULT_ALREADY_INSTALLED,
    RESULT_FAILED,
    RESULT_CRASHED,
    RESULT_LAUNCH_ERROR,
    RESULT_EXTRACT_ERROR
};

static InstallResult InstallComponent(int index, const std::string& workDir) {
    const Component& comp = g_components[index];

    printf("============================================================\n");
    printf("  %s\n", comp.displayName);
    printf("============================================================\n");

    // 1. 检测是否已安装
    std::string detail;
    if (IsComponentInstalled(index, detail)) {
        printf("  [已安装] %s 已安装，无需安装。\n", comp.displayName);
        printf("           %s\n\n", detail.c_str());
        return RESULT_ALREADY_INSTALLED;
    }

    // 2. 释放安装包
    std::string outPath = workDir + comp.fileName;
    printf("  -> 正在释放安装包到临时目录...\n");
    if (!ExtractResource(comp.resId, outPath)) {
        printf("  [安装失败] 资源释放失败！\n\n");
        return RESULT_EXTRACT_ERROR;
    }
    printf("  -> 释放完成: %s\n", outPath.c_str());

    // 3. 运行安装程序
    printf("  -> 正在启动安装程序，请在弹出的安装向导中完成安装...\n");
    printf("     (如需取消，请在安装向导中点击取消)\n\n");

    DWORD exitCode = RunAndWait(outPath);

    // 4. 立即删除临时文件
    DeleteFileA(outPath.c_str());

    // 5. 判断结果
    if (exitCode == 0xFFFFFFFF) {
        printf("  [安装失败] 无法启动安装程序！\n\n");
        return RESULT_LAUNCH_ERROR;
    }
    if (exitCode == 0) {
        printf("  [安装成功] %s 已完成安装。\n\n", comp.displayName);
        return RESULT_SUCCESS;
    }
    if (exitCode >= 0x80000000) {
        printf("  [安装失败] 安装程序意外终止！退出码: 0x%08lX\n", exitCode);
        printf("             可能原因: 程序崩溃、被任务管理器结束等。\n\n");
        return RESULT_CRASHED;
    }

    // 非0退出码，再次检测是否实际安装成功
    if (IsComponentInstalled(index, detail)) {
        printf("  [安装成功] %s 已完成安装 (退出码: %lu，检测到已安装)。\n\n",
               comp.displayName, exitCode);
        return RESULT_SUCCESS;
    }

    printf("  [安装失败] 安装程序返回错误码: %lu\n", exitCode);
    printf("             可能原因: 用户取消安装、安装包损坏、权限不足等。\n\n");
    return RESULT_FAILED;
}

// ============================================================
// 主函数
// ============================================================

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    printf("============================================================\n");
    printf("   STM32 开发环境一键安装程序 (stm install)\n");
    printf("   CH340驱动 + JRE 8 + STM32CubeIDE 1.17.0\n");
    printf("============================================================\n\n");

    // 检查管理员权限
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elev = {0};
        DWORD retLen = 0;
        if (GetTokenInformation(hToken, TokenElevation, &elev, sizeof(elev), &retLen)) {
            isAdmin = elev.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    if (!isAdmin) {
        printf("[提示] 建议以管理员身份运行，否则部分驱动可能安装失败。\n\n");
    }

    // 显示当前安装状态
    printf("  当前安装状态:\n");
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        std::string detail;
        bool installed = IsComponentInstalled(i, detail);
        printf("    [%d] %s: %s\n", i + 1, g_components[i].displayName,
               installed ? "已安装" : "未安装");
    }
    printf("\n");

    printf("============================================================\n");
    printf("  开始安装 (已安装的组件将自动跳过)...\n");
    printf("============================================================\n\n");

    std::string workDir = GetTempDir() + "stm_installer\\";
    CreateDirectoryA(workDir.c_str(), NULL);

    int successCount = 0;
    int alreadyCount = 0;
    int failCount = 0;

    for (int i = 0; i < NUM_COMPONENTS; i++) {
        printf("[%d/%d] ", i + 1, NUM_COMPONENTS);
        InstallResult result = InstallComponent(i, workDir);

        switch (result) {
            case RESULT_SUCCESS:          successCount++; break;
            case RESULT_ALREADY_INSTALLED: alreadyCount++; break;
            default:                      failCount++; break;
        }
        // 已安装的直接跳过，不询问
        // 失败的也直接继续下一个，不询问
    }

    // 清理临时目录
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        DeleteFileA((workDir + g_components[i].fileName).c_str());
    }
    RemoveDirectoryA(workDir.c_str());

    printf("============================================================\n");
    printf("  安装结束\n");
    printf("  新安装: %d  已安装跳过: %d  失败: %d\n", successCount, alreadyCount, failCount);
    if (failCount == 0 && (successCount + alreadyCount) == NUM_COMPONENTS) {
        printf("  全部组件就绪！\n");
    } else if (failCount > 0) {
        printf("  有 %d 个组件安装失败，请检查上方日志。\n", failCount);
    }
    printf("============================================================\n\n");

    printf("按回车键退出...");
    getchar();
    return 0;
}
