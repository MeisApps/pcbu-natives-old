#include <string>
#include "deps/pstream.h"

#define PAM_SM_AUTH
#include <security/pam_modules.h>

#ifdef LINUX
#define PCBU_AUTH_PATH "/usr/sbin/pcbu_auth"
#endif
#ifdef APPLE
#include <security/pam_appl.h>
#define PCBU_AUTH_PATH "/usr/local/sbin/pcbu_auth"
#endif

static int print_pam(struct pam_conv *conv, const std::string& message) {
    struct pam_message msg{};
    struct pam_response *resp = nullptr;
    const struct pam_message* pMsg = &msg;

    if(!conv)
        return PAM_CONV_ERR;

    msg.msg = (char *)message.c_str();
    msg.msg_style = PAM_TEXT_INFO;
    conv->conv(1, &pMsg, &resp, conv->appdata_ptr);
    return PAM_SUCCESS;
}

int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    const char* userName = nullptr;
    const char* serviceName = nullptr;
    struct pam_conv *conv = nullptr;

    auto statusCode = pam_get_item(pamh, PAM_SERVICE, (const void **)&serviceName);
    if (statusCode == PAM_SUCCESS) {
        statusCode = pam_get_user(pamh, (const char **)&userName, nullptr);
        if(statusCode == PAM_SUCCESS) {
            statusCode = pam_get_item(pamh, PAM_CONV, (const void **) &conv);
        }
    }
    if (statusCode != PAM_SUCCESS) {
        //Logger::println(I18n::Get("error_pam"));
        return PAM_IGNORE;
    }

    std::vector<std::string> args;
    args.emplace_back(PCBU_AUTH_PATH);
    args.emplace_back(userName);
    args.emplace_back(serviceName);

    redi::ipstream proc(PCBU_AUTH_PATH, args, redi::pstreams::pstdout);
    std::string line;
    while (std::getline(proc.out(), line))
        print_pam(conv, line);

    auto result = proc.close();
    if(result == 0)
        return PAM_SUCCESS;
    else if(result == 1)
        return PAM_AUTH_ERR;

    return PAM_IGNORE;
}

int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}
