#ifndef _H_KV_PAIR_
#define _H_KV_PAIR_

template <typename T>
struct kv_pair {
    const char *key;
    T val;
};

template <typename T>
static T match_kv( kv_pair<T> *kv, const char *key)
{
    while(kv->key != NULL) {
        if (strcmp(key,kv->key)==0) break;
        kv++;
    }
    return kv->val;
}

#endif // _H_KV_PAIR_