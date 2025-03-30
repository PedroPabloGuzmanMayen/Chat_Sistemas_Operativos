#include "protocol.h"

namespace Protocol {

    void append_string(std::vector<uint8_t>& buffer, const std::string& str) {
        buffer.push_back(static_cast<uint8_t>(str.size()));
        buffer.insert(buffer.end(), str.begin(), str.end());
    }

    std::string extract_string(const std::vector<uint8_t>& data, size_t& offset) {
        uint8_t len = data[offset++];
        std::string result(data.begin() + offset, data.begin() + offset + len);
        offset += len;
        return result;
    }

    std::vector<uint8_t> make_list_users() {
        return { LIST_USERS };
    }

    std::vector<uint8_t> make_get_user_info(const std::string& username) {
        std::vector<uint8_t> msg = { GET_USER_INFO };
        append_string(msg, username);
        return msg;
    }

    std::vector<uint8_t> make_change_status(const std::string& username, UserStatus status) {
        std::vector<uint8_t> msg = { CHANGE_STATUS };
        append_string(msg, username);
        msg.push_back(static_cast<uint8_t>(status));
        return msg;
    }

    std::vector<uint8_t> make_send_message(const std::string& to, const std::string& message) {
        std::vector<uint8_t> msg = { SEND_MESSAGE };
        append_string(msg, to);
        append_string(msg, message);
        return msg;
    }

    std::vector<uint8_t> make_get_history(const std::string& chat_name) {
        std::vector<uint8_t> msg = { GET_HISTORY };
        append_string(msg, chat_name);
        return msg;
    }
}