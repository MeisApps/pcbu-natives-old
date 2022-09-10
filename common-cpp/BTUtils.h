#ifndef PAM_PCBIOUNLOCK_BTUTILS_H
#define PAM_PCBIOUNLOCK_BTUTILS_H

#include <string>

#ifndef _WIN32

#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#endif


class BTUtils {
public:
#ifndef _WIN32
    static int FindChannelSDP(const std::string& deviceAddress, uint8_t *uuid) {
        bdaddr_t address;
        str2ba(deviceAddress.c_str(), &address);

        auto bdAny = (bdaddr_t){ {0, 0, 0, 0, 0, 0} };
        sdp_session_t* session = sdp_connect(&bdAny, &address, SDP_RETRY_IF_BUSY);
        if (!session) {
            return -1;
        }

        uuid_t uuid128;
        sdp_uuid128_create(&uuid128, uuid);

        // create query lists
        int range = 0x0000ffff;
        sdp_list_t* responseList;
        sdp_list_t* searchList = sdp_list_append(nullptr, &uuid128);
        sdp_list_t* attrIdList = sdp_list_append(nullptr, &range);

        // search for records
        int success = sdp_service_search_attr_req(
            session, searchList, SDP_ATTR_REQ_RANGE, attrIdList, &responseList);
        if (success) {
            return -1;
        }

        // check responses
        success = sdp_list_len(responseList);
        if (success <= 0) {
            return -1;
        }

        // process responses
        int channel = 0;
        sdp_list_t* responses = responseList;
        while (responses) {
            auto record = (sdp_record_t*)responses->data;

            sdp_list_t* protoList;
            success = sdp_get_access_protos(record, &protoList);
            if (success) {
                return -1;
            }

            sdp_list_t* protocol = protoList;
            while (protocol) {
                sdp_list_t* pds;
                int protocolCount = 0;
                pds = (sdp_list_t*)protocol->data;

                while (pds) { // loop thru all pds
                    sdp_data_t* d;
                    int dtd;
                    d = (sdp_data_t*)(pds->data);
                    while (d) {
                        dtd = d->dtd;
                        switch (dtd) {
                        case SDP_UUID16:
                        case SDP_UUID32:
                        case SDP_UUID128:
                            protocolCount = sdp_uuid_to_proto(&d->val.uuid);
                            break;
                        case SDP_UINT8:
                            if (protocolCount == RFCOMM_UUID) {
                                channel = d->val.uint8; // save channel id
                            }
                            break;
                        default:
                            break;
                        }
                        d = d->next; // to next data unit
                    }
                    pds = pds->next; // to next pds
                }
                sdp_list_free((sdp_list_t*)protocol->data, nullptr);

                protocol = protocol->next; // to next protocol
            }
            sdp_list_free(protoList, nullptr);

            responses = responses->next; // to next response
        }

        if (channel <= 0 || channel > 30)
            return -1;
        return channel;

    }
#endif
private:
    BTUtils() = default;
};

#endif //PAM_PCBIOUNLOCK_BTUTILS_H
