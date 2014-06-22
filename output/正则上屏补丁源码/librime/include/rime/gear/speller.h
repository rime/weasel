//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-10-27 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SPELLER_H_
#define RIME_SPELLER_H_

#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

//正则补丁
#include <boost/regex.hpp>
//end

namespace rime
{

class Context;
struct Segment;

class Speller : public Processor
{
public:
    Speller(const Ticket& ticket);

    virtual ProcessResult ProcessKeyEvent(const KeyEvent& key_event);

protected:
    bool AutoSelectAtMaxCodeLength(Context* ctx);
    bool AutoSelectUniqueCandidate(Context* ctx);
    bool AutoSelectPreviousMatch(Context* ctx, Segment* previous_segment);
    bool FindEarlierMatch(Context* ctx, size_t start, size_t end);
    bool MaxNextCode(Context* ctx, Segment* previous_segment);

    std::string alphabet_;
    std::string delimiters_;
    std::string initials_;
    std::string finals_;
    int max_code_length_;
    bool auto_select_;
    bool use_space_;

    //正则补丁
    std::string auto_select_unique_pattern_, auto_select_maxcode_excludepattern_;
    bool check_max_next_code_;
    //end

};

}  // namespace rime

#endif  // RIME_SPELLER_H_

