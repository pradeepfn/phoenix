//
// Created by pradeep on 8/30/17.
//

#include <cstdint>
#include "nvs/log.h"
#include "nvs_store.h"
#include "logentry.h"

#define LOG_SIZE 10 * 1024 * 1024 * 30LLU

namespace nvs{

    NVSStore::NVSStore(std::string storeId):
            storeId(storeId)
    {
        ErrorCode ret;
        PoolId poolId;

        std::string delimeter = "/";
        std::string heapId =  storeId.substr(0,storeId.find(delimeter));
        std::string plid = storeId.substr(storeId.find(delimeter) + delimeter.length());
        poolId = std::stoi(plid);

        MemoryManager *mm = MemoryManager::GetInstance();
        ret = mm->FindLog(poolId, &(this->log));
        if(ret == ID_NOT_FOUND){
            ret = mm->CreateLog(poolId, LOG_SIZE);
            if(ret != NO_ERROR){
                LOG(fatal) << "NVSStore: error creating log";
                exit(1);
            }
            ret = mm->FindLog(poolId, &(this->log));
            if(ret != NO_ERROR){
                LOG(fatal) << "NVSStore: error finding log";
                exit(1);
            }
        }
    }

    NVSStore::~NVSStore() {



    }

    ErrorCode NVSStore::create_obj(std::string key, uint64_t size, void **obj_addr)
    {

        LOG(debug) << "Object created : " + key;
        void *tmp_ptr = malloc(size);
        Object *obj = new Object(key,size,0,tmp_ptr);
        std::map<std::string, Object *>::iterator it =   objectMap.find(key);
        if(it == objectMap.end()){
            obj->setVersion(0);
            objectMap[key] = obj;
        }else{
            LOG(fatal) << "not implemented yet";
            exit(1);
        }
        *obj_addr = (uint64_t *)tmp_ptr;

        return NO_ERROR;
    }


    /*
     * this is similar to a commit operation. we do not need the object
     * address and size for put operation as the kvstore already know that
     * detail.
     */
    ErrorCode NVSStore::put(std::string key, uint64_t version)
    {
            struct iovec *iovp, *next_iovp;
            ErrorCode ret;
            struct logentry l_entry;

            std::map<std::string, Object *>::iterator it;
            // first traverse the map and find the key object
            if((it = objectMap.find(key)) != objectMap.end()){
                Object *obj = it->second;
                //uint64_t version = obj->getVersion();

                //construct IO vector
                int iovcnt = 2; // data + header
                iovp = (struct iovec *)malloc(sizeof(struct iovec) * iovcnt);

                next_iovp = iovp;
                //populate header
                l_entry.version = version;
                snprintf(l_entry.key, 64,"%s", obj->getName().c_str());
                l_entry.len = obj->getSize();

                next_iovp->iov_base = &l_entry;
                next_iovp->iov_len = sizeof(l_entry);
                next_iovp++;
                //data

                next_iovp->iov_base = obj->getPtr();
                next_iovp->iov_len = obj->getSize();


                ret = this->log->appendv(iovp,iovcnt);
                if(ret != NO_ERROR){
                    LOG(fatal) << "Store: append failed";
                    exit(1);
                }
                free(iovp);
                //increase the soft state
                obj->setVersion(version);
                return NO_ERROR;
            }
            LOG(error) << "element not found";
            return ELEM_NOT_FOUND;
    }


    /*
     * implements the bulk snapshot method.
     * TODO: use our own log. PmemLog does not suport multi-process data edits
     * TODO: optimize with only two fences per bulk write
     */
    ErrorCode NVSStore::put_all() {

        struct iovec *iovp, *next_iovp;
        ErrorCode ret;
        struct logentry l_entry;

        std::map<std::string, Object *>::iterator it;
        for(it = objectMap.begin(); it != objectMap.end(); it++){
            Object *obj = it->second;
            uint64_t version = obj->getVersion()+1;

            //construct IO vector
            int iovcnt = 2; // data + header
            iovp = (struct iovec *)malloc(sizeof(struct iovec) * iovcnt);

            next_iovp = iovp;
            //populate header
            l_entry.version = version;
            snprintf(l_entry.key, 64,"%s", obj->getName().c_str());
            l_entry.len = obj->getSize();

            next_iovp->iov_base = &l_entry;
            next_iovp->iov_len = sizeof(l_entry);
            next_iovp++;
            //data

            next_iovp->iov_base = obj->getPtr();
            next_iovp->iov_len = obj->getSize();


            ret = this->log->appendv(iovp,iovcnt);
            LOG(debug)<< "Object  - " << obj->getName() << " saved on successfully";
            if(ret != NO_ERROR){
                LOG(fatal) << "Store: append failed";
                exit(1);
            }
            free(iovp);
            //increase the soft state
            obj->setVersion(version);
        }
        return NO_ERROR;
    }



    static int
    processLog(const void *buf, size_t len, void *arg)
    {
        // getting result from the callback -- walkentry structure
        struct walkentry *wentry = (struct walkentry *) arg;

        const void *endp = (char *)buf + len;
        buf = (char *)buf + wentry->start_offset;
        while(buf < endp){
            struct logentry *headerp = (struct logentry *) buf;
            buf = (char *)buf + sizeof(struct logentry);

            //check for key and version
            if(!strncmp(headerp->key, wentry->key,64) && headerp->version == wentry->version){
                wentry->datap = (void *)buf;
                wentry->len = headerp->len;
                wentry->err = NO_ERROR;
                return 0; // no error
            }else{
                LOG(debug) << "looking for : " + std::string(wentry->key) + " , " +  std::to_string(wentry->version) +
                              "   current entry : " + std::string(headerp->key) + " , " + std::to_string(headerp->version);
            }
            buf = (char *) buf + headerp->len;
        }
        wentry->err = ID_NOT_FOUND;
        return -1;
    }



    /*
     *  the object address is redundant here
     */
    ErrorCode NVSStore::get(std::string key, uint64_t version, void *obj_addr)
    {
        struct walkentry wentry;
        wentry.version = version;
        wentry.start_offset = 0;
        snprintf(wentry.key,64,"%s",key.c_str());

        this->log->walk(processLog, &wentry);

        if(wentry.err != NO_ERROR){
            LOG(debug) << "Object not found : " + key + " , " + std::to_string(version);
            return ID_NOT_FOUND;
        }

        //*obj_addr = wentry.datap;
        memcpy(obj_addr,wentry.datap,wentry.len);
        return NO_ERROR;

    }

    ErrorCode NVSStore::get_with_malloc(std::string key, uint64_t version, void **addr) {

        struct walkentry wentry;
        wentry.version = version;
        wentry.start_offset = 0;
        snprintf(wentry.key,64,"%s",key.c_str());

        this->log->walk(processLog, &wentry);

        if(wentry.err != NO_ERROR){
            LOG(debug) << "Object not found : " + key + " , " + std::to_string(version);
            return ID_NOT_FOUND;
        }

        //find the object from the object map, if not found -- > create one
        std::map<std::string, Object *>::iterator it;
        Object *obj;
        if((it=objectMap.find(key)) != objectMap.end()){
            obj = it->second;
        }else{
            void *tmp_ptr = malloc(wentry.len);
            obj = new Object(key,wentry.len,version,tmp_ptr);
            objectMap[key] = obj;
        }
        obj->setVersion(version);
        //*obj_addr = wentry.datap;
        memcpy(obj->getPtr(),wentry.datap,wentry.len);
        *addr = obj->getPtr();
        return NO_ERROR;
    }

    Key * NVSStore::findKey(std::string key) {

    }


    void NVSStore::stats() {

    }

}
