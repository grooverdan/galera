#ifndef EVS_MESSAGE_H
#define EVS_MESSAGE_H

#include <cstdlib>

#include "gcomm/exception.hpp"
#include "gcomm/address.hpp"
#include "gcomm/logger.hpp"

#include "evs_seqno.hpp"

#include <map>

typedef Address EVSPid;

class EVSViewId {
    uint8_t uuid[8];
public:
    EVSViewId() {
	memset(uuid, 0, 8);
    }
    EVSViewId(const EVSPid& sa, const uint32_t seq) {
	write_uint32(::rand(), uuid, sizeof(uuid), 0);
	write_uint32(seq, uuid, sizeof(uuid), 4);
    }

    uint32_t get_seq() const {
	uint32_t ret;
	if (read_uint32(uuid, sizeof(uuid), 4, &ret) == 0)
	    throw FatalException("");
	return ret;
    }
    
    size_t read(const void* buf, const size_t buflen, const size_t offset) {
	if (offset + 8 > buflen)
	    return 0;
	memcpy(uuid, (const uint8_t*)buf + offset, 8);
	return offset + 8;
    }

    size_t write(void* buf, const size_t buflen, const size_t offset) const {
	if (offset + 8 > buflen)
	    return 0;
	memcpy((uint8_t*)buf + offset, uuid, 8);
	return offset + 8;
    }

    size_t size() const {
	return 8;
    }

    bool operator<(const EVSViewId& cmp) const {
	return memcmp(uuid, cmp.uuid, 8) < 0;
    }
    bool operator==(const EVSViewId& cmp) const {
	return memcmp(uuid, cmp.uuid, 8) == 0;
    }
};


struct EVSRange {
    uint32_t low;
    uint32_t high;
    EVSRange() : low(SEQNO_MAX), high(SEQNO_MAX) {}
    EVSRange(const uint32_t low_, const uint32_t high_) :
	low(low_), high(high_) {}
    uint32_t get_low() const {
	return low;
    }
    uint32_t get_high() const {
	return high;
    }
    bool operator==(const EVSRange& cmp) const {
	return cmp.get_low() == low && cmp.get_high() == high;
    }
};

struct EVSGap {
    EVSPid source;
    EVSRange range;
    EVSGap() {}
    EVSGap(const EVSPid& source_, const EVSRange& range_) :
	source(source_), range(range_) {}

    EVSPid get_source() const {
	return source;
    }
    uint32_t get_low() const {
	return range.low;
    }

    uint32_t get_high() const {
	return range.high;
    }
};

    enum EVSSafetyPrefix {
	DROP,
	UNRELIABLE,
	FIFO,
	AGREED,
	SAFE
    };


class EVSMessage {
public:
    
    enum Type {
	USER,     // User message
	DELEGATE, // Message that has been sent on behalf of other instace 
	GAP,      // Message containing seqno arrays and/or gaps
	JOIN,     // Join message
	LEAVE,    // Leave message
	INSTALL   // Install message
    };


    enum Flag {
	F_MSG_MORE = 0x1
    };

private:
    int version;
    Type type;
    EVSSafetyPrefix safety_prefix;
    uint32_t seq;
    uint8_t seq_range;
    uint32_t aru_seq;
    uint8_t flags;
    EVSViewId source_view;
    EVSPid source;
    EVSGap gap;
    std::map<EVSPid, EVSRange>* oper_inst;
    std::set<EVSPid>* untr_inst;
    std::set<EVSPid>* unop_inst;
public:    


    EVSMessage() : seq(SEQNO_MAX) {}
    
    // User message
    EVSMessage(const Type type_, 
	       const EVSSafetyPrefix sp_, 
	       const uint32_t seq_, 
	       const uint8_t seq_range_,
	       const uint32_t aru_seq_,
	       const EVSViewId vid_, const uint8_t flags_) : 
	version(0), 
	type(type_), 
	safety_prefix(sp_), 
	seq(seq_), 
	seq_range(seq_range_),
	aru_seq(aru_seq_),
	flags(flags_),
	source_view(vid_), 
	oper_inst(0), 
	untr_inst(0), 
	unop_inst(0) {
	if (type != USER)
	    throw FatalException("Invalid type");
    }
    
    // Delegate message
    EVSMessage(const Type type_, const EVSPid& source_) :
	type(type_), 
	source(source_), 
	oper_inst(0), 
	untr_inst(0), 
	unop_inst(0) {
	if (type != DELEGATE)
	    throw FatalException("Invalid type");
    }
    
    // Gap message
    EVSMessage(const Type type_, const uint32_t seq_, const EVSGap& gap_) :
	type(type_), 
	seq(seq_), 
	gap(gap_),
	oper_inst(0), 
	untr_inst(0), 
	unop_inst(0) {
	if (type != GAP)
	    throw FatalException("Invalid type");
    } 

    // Join and install messages
    EVSMessage(const Type type_, const EVSViewId& vid_, 
	       const uint32_t aru_seq_, const uint32_t safe_seq_) :
	type(type_),
	seq(safe_seq_),
	aru_seq(aru_seq_),
	source_view(vid_)  {
	if (type != JOIN && type != INSTALL)
	    throw FatalException("Invalid type");
	oper_inst = new std::map<EVSPid, EVSRange>;
	untr_inst = new std::set<EVSPid>;
	unop_inst = new std::set<EVSPid>;
    }
    
    // Leave message
    EVSMessage(const Type type_, const EVSViewId& vid_) :
	type(type_),
	source_view(vid_) {
	if (type != LEAVE)
	    throw FatalException("Invalid type");
    }


	
    
    ~EVSMessage() {
	delete oper_inst;
	delete untr_inst;
	delete unop_inst;
    }
    
    Type get_type() const {
	return type;
    }
    
    EVSSafetyPrefix get_safety_prefix() const {
	return safety_prefix;
    }
    
    EVSPid get_source() const {
	return source;
    }

    uint32_t get_seq() const {
	return seq;
    }
    
    uint8_t get_seq_range() const {
	return seq_range;
    }

    uint32_t get_aru_seq() const {
	return aru_seq;
    }

    uint8_t get_flags() const {
	return flags;
    }
    
    EVSViewId get_source_view() const {
	return source_view;
    }

    EVSGap get_gap() const {
	return gap;
    }

    const std::map<EVSPid, EVSRange>* get_operational() const {
	return oper_inst;
    }

    void add_operational_instance(const EVSPid& pid, const EVSRange& range) {
	std::pair<std::map<EVSPid, EVSRange>::iterator, bool> i = 
	    oper_inst->insert(std::pair<EVSPid, EVSRange>(pid, range));
	if (i.second == false)
	    throw FatalException("");
    }

    void add_untrusted_instance(const EVSPid& pid) {
	std::pair<std::set<EVSPid>::iterator, bool> i = 
	    untr_inst->insert(pid);
	if (i.second == false)
	    throw FatalException("");
    }

    void add_unoperational_instance(const EVSPid& pid) {
	std::pair<std::set<EVSPid>::iterator, bool> i = 
	    unop_inst->insert(pid);
	if (i.second == false)
	    throw FatalException("");
    }
    
    
    // Message serialization:

    size_t read(const void* buf, const size_t buflen, const size_t offset) {
	uint8_t b;
	size_t off;
	if ((off = read_uint8(buf, buflen, offset, &b)) == 0)
	    return 0;
	version = b & 0xf;
	type = static_cast<Type>((b >> 4) & 0xf);
	if (type == USER) {
	    if ((off = read_uint8(buf, buflen, off, &b)) == 0)
		return 0;
	    safety_prefix = static_cast<EVSSafetyPrefix>(b & 0xf);
	    if ((off = read_uint8(buf, buflen, off, &seq_range)) == 0)
		return 0;
	    if ((off = read_uint8(buf, buflen, off, &flags)) == 0)
		return 0;
	    if ((off = read_uint32(buf, buflen, off, &seq)) == 0)
		return 0;
	    if ((off = read_uint32(buf, buflen, off, &aru_seq)) == 0)
		return 0;
	    if ((off = source_view.read(buf, buflen, off)) == 0)
		return 0;
	} else if (type == DELEGATE) {
	    off += 3; // Skip padding
	    if (source.read(buf, buflen, off) == 0)
		return 0;
	}
	return off;
    }
    
    size_t write(void* buf, const size_t buflen, const size_t offset) const {
	uint8_t b;
	size_t off;
	
	b = (version & 0xf) | ((type << 4) & 0xf0);
	if ((off = write_uint8(b, buf, buflen, offset)) == 0)
	    return 0;
	if (type == USER) {
	    b = safety_prefix & 0xf;	
	    if ((off = write_uint8(b, buf, buflen, off)) == 0)
		return 0;
	    if ((off = write_uint8(seq_range, buf, buflen, off)) == 0)
		return 0;
	    if ((off = write_uint8(flags, buf, buflen, off)) == 0)
		return 0;
	    if ((off = write_uint32(seq, buf, buflen, off)) == 0)
		return 0;
	    if ((off = write_uint32(aru_seq, buf, buflen, off)) == 0)
		return 0;
	    if ((off = source_view.write(buf, buflen, off)) == 0)
		return 0;
	} else if (type == DELEGATE) {
	    for (int i = 0; i < 3; ++i)
		if ((off = write_uint8(0, buf, buflen, off)) == 0)
		    return 0;
	    if ((off = source.write(buf, buflen, off)) == 0)
		return 0;
	}
	return off;
    }
    
    size_t size() const {
	if (type == EVSMessage::USER)
	    return 4 + 4 + 4 + source_view.size(); // bits + seq + aru_seq + view
	if (type == DELEGATE)
	    return 4 + source.size(); // 
	return 0;
    }

    mutable unsigned char hdrbuf[32];
    const void* get_hdr() const {
	if (write(hdrbuf, sizeof(hdrbuf), 0) == 0)
	    throw FatalException("Short buffer");
	return hdrbuf;
    }

    size_t get_hdrlen() const {
	return size();
    }
    
};

// Compare two evs messages
inline bool equal(const EVSMessage* a, const EVSMessage* b)
{
    if (a->get_type() != b->get_type())
	return false;
    switch (a->get_type()) {

    case EVSMessage::JOIN:

    default:
	LOG_DEBUG(std::string("equal() not implemented for ") + ::to_string(a->get_type()));
    }
    return false;
}

#endif // EVS_MESSAGE
