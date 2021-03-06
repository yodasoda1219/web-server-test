#include "login_panel.h"
//#include <curl/curl.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <api-standard.h>
#include "../util.h"
#include "../application.h"
#include "settings_panel.h"
#include <cassert>
namespace guifrontend {
    namespace panels {
        login_panel::login_panel(size_t settings_index) {
            this->m_settings_panel_index = settings_index;
            this->m_status = status::IDLE;
        }
        void login_panel::render() {
            ImGui::Begin("Log in to the server", &this->m_visible);
            std::string statusname;
            switch (this->m_status) {
            case status::IDLE:
                statusname = "Idle";
                break;
            case status::LOGGED_IN:
                statusname = "Logged in";
                break;
            case status::FAILED:
                statusname = "Verification failed";
                break;
            case status::USER_NOT_FOUND:
                statusname = "User not found";
                break;
            case status::NO_SERVER:
                statusname = "No server address";
                break;
            default:
                statusname = "Unknown";
                break;
            }
            ImGui::Text("Status: %s", statusname.c_str());
            static std::string displayname, password;
            ImGui::InputText("Username", &displayname);
            ImGui::InputText("Password", &password, ImGuiInputTextFlags_Password);
            auto settings = this->m_parent->get_panel(this->m_settings_panel_index);
            std::string address = ((settings_panel*)settings.get())->get_settings().server_address;
            if (ImGui::Button("Login")) {
                this->send_request(displayname, password, address);
                displayname.clear();
                password.clear();
            }
            ImGui::SameLine();
            if (ImGui::Button("Register")) {
                if (!address.empty()) {
                    if (!displayname.empty()) {
                        apistandard::newuser nu;
                        nu.displayname = displayname;
                        nu.password = password;
                        nlohmann::json json_data = nu;
                        // we dont care about the response for now
                        util::request(util::request_type::POST, address + "/user/new", { { "Content-Type", "application/json" } }, json_data.dump());
                        this->send_request(displayname, password, address);
                        displayname.clear();
                        password.clear();
                    }
                }
                else {
                    this->m_status = status::NO_SERVER;
                }
            }
            ImGui::End();
        }
        login_panel::login login_panel::get_login() {
            return (this->m_status == status::LOGGED_IN) ? this->m_login : login{ (size_t)-1, "" };
        }
        void login_panel::send_request(const std::string& displayname, const std::string& password, const std::string& address) {
            if (address.empty()) {
                this->m_status = status::NO_SERVER;
                return;
            }
            size_t id = util::find_user(displayname, address);
            if (id == (size_t)-1) {
                this->m_status = status::USER_NOT_FOUND;
                return;
            }
            apistandard::login login;
            login.id = id;
            login.password = password;
            nlohmann::json json_data = login;
            auto response = util::request(util::request_type::POST, address + "/user/verify", { { "Content-Type", "application/json" } }, json_data.dump());
            assert(response.code == 200);
            json_data = nlohmann::json::parse(response.data);
            auto verification = json_data.get<apistandard::login_verification>();
            if (verification.is_valid) {
                this->m_login.id = login.id;
                this->m_login.password = login.password;
                this->m_status = status::LOGGED_IN;
            } else {
                this->m_status = status::FAILED;
            }
        }
    }
}