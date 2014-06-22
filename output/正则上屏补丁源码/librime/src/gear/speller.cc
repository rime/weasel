//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-10-27 GONG Chen <chen.sst@gmail.com>
//
#include <utility>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/menu.h>
#include <rime/schema.h>
#include <rime/gear/speller.h>

static const char kRimeAlphabet[] = "zyxwvutsrqponmlkjihgfedcba";

namespace rime
{

static inline bool belongs_to(char ch, const std::string& charset)
{
    return charset.find(ch) != std::string::npos;
}

static bool reached_max_code_length(const shared_ptr<Candidate>& cand,
                                    int max_code_length)
{
    if (!cand)
        return false;
    int code_length = static_cast<int>(cand->end() - cand->start());
    return code_length >= max_code_length;
}

static bool is_auto_selectable(const shared_ptr<Candidate>& cand,
                               const std::string& input,
                               const std::string& delimiters)
{
    return
        // reaches end of input
        cand->end() == input.length() &&
        // is table entry
        Candidate::GetGenuineCandidate(cand)->type() == "table" &&
        // no delimiters
        input.find_first_of(delimiters, cand->start()) == std::string::npos;
}

static bool expecting_an_initial(Context* ctx,
                                 const std::string& alphabet,
                                 const std::string& finals)
{
    size_t caret_pos = ctx->caret_pos();
    if (caret_pos == 0)
        return true;
    const std::string& input(ctx->input());
    //assert(input.length() >= caret_pos);
    char previous_char = input[caret_pos - 1];
    return belongs_to(previous_char, finals) ||
           !belongs_to(previous_char, alphabet);
}

Speller::Speller(const Ticket& ticket) : Processor(ticket),
    alphabet_(kRimeAlphabet),
    max_code_length_(0),
    auto_select_(false),
    use_space_(false),
    check_max_next_code_(false)
{
    if (Config* config = engine_->schema()->config()) {
        config->GetString("speller/alphabet", &alphabet_);
        config->GetString("speller/delimiter", &delimiters_);
        config->GetString("speller/initials", &initials_);
        config->GetString("speller/finals", &finals_);
        config->GetInt("speller/max_code_length", &max_code_length_);
        config->GetBool("speller/auto_select", &auto_select_);
        config->GetBool("speller/use_space", &use_space_);
        //正则补丁
        config->GetString("speller/auto_select_unique_pattern", &auto_select_unique_pattern_);
        config->GetString("speller/auto_select_maxcode_excludepattern", &auto_select_maxcode_excludepattern_);
        config->GetBool("speller/check_max_next_code", &check_max_next_code_);
        //end
    }
    if (initials_.empty()) {
        initials_ = alphabet_;
    }
}


ProcessResult Speller::ProcessKeyEvent(const KeyEvent& key_event)
{
    if (key_event.release() || key_event.ctrl() || key_event.alt())
        return kNoop;
    int ch = key_event.keycode();
    if (ch < 0x20 || ch >= 0x7f)  // not a valid key for spelling
        return kNoop;
    if (ch == XK_space && (!use_space_ || key_event.shift()))
        return kNoop;
    if (!belongs_to(ch, alphabet_) && !belongs_to(ch, delimiters_))
        return kNoop;
    Context* ctx = engine_->context();
    bool is_initial = belongs_to(ch, initials_);
    if (!is_initial &&
        expecting_an_initial(ctx, alphabet_, finals_)) {
        return kNoop;
    }
    //initials max+1 code补丁
    if (!is_initial && belongs_to(ch, alphabet_) && AutoSelectAtMaxCodeLength(ctx)) {
        DLOG(INFO) << "auto-select at max code length.";
        return kNoop;
    }
    if (is_initial && !check_max_next_code_ && AutoSelectAtMaxCodeLength(ctx)) {
        //end
        DLOG(INFO) << "auto-select at max code length.";
    }
    // make a backup of previous conversion before modifying input
    Segment previous_segment;
    if (auto_select_ && ctx->HasMenu()) {
        previous_segment = ctx->composition()->back();
    }
    DLOG(INFO) << "add to input: '" << (char)ch << "', " << key_event.repr();
    ctx->PushInput(key_event.keycode());
    ctx->ConfirmPreviousSelection();  // so that next BackSpace won't revert
    // previous selection
    if (AutoSelectPreviousMatch(ctx, &previous_segment)) {
        DLOG(INFO) << "auto-select previous match.";
    }

    if (AutoSelectUniqueCandidate(ctx)) {
        DLOG(INFO) << "auto-select unique candidate.";
    }

    //探测非initials max+1 code补丁
    if (MaxNextCode(ctx, &previous_segment)) {
        DLOG(INFO) << "auto-select maxnextcode match.";
        ctx->PushInput(key_event.keycode());
    }
    //end
    return kAccepted;
}

bool Speller::AutoSelectAtMaxCodeLength(Context* ctx)
{
    if (max_code_length_ <= 0)
        return false;
    if (!ctx->HasMenu())
        return false;
    const Segment& seg(ctx->composition()->back());
    auto cand = seg.GetSelectedCandidate();
    if (cand &&
        reached_max_code_length(cand, max_code_length_) &&
        is_auto_selectable(cand, ctx->input(), delimiters_)) {
        //maxcode顶排排除模式补丁
        if (auto_select_maxcode_excludepattern_.empty() || !boost::regex_match(ctx->input(), boost::regex(auto_select_maxcode_excludepattern_))) {
            ctx->ConfirmCurrentSelection();
        //end
            return true;
        }
    }
    return false;
}

bool Speller::AutoSelectUniqueCandidate(Context* ctx)
{
    if (!auto_select_)
        return false;
    if (!ctx->HasMenu())
        return false;
    const Segment& seg(ctx->composition()->back());
    bool unique_candidate = seg.menu->Prepare(2) == 1;
    if (!unique_candidate)
        return false;
    auto cand = seg.GetSelectedCandidate();
    std::string input = ctx->input();
    //正则补丁
    if ((( auto_select_unique_pattern_.empty() &&
           (max_code_length_ == 0 ||  // at any length if not specified
            reached_max_code_length(cand, max_code_length_))) ||
         (!auto_select_unique_pattern_.empty() && boost::regex_match(input, boost::regex(auto_select_unique_pattern_))))
        && is_auto_selectable(cand, input, delimiters_))
        //end
    {
        ctx->ConfirmCurrentSelection();
        return true;
    }
    return false;
}

bool Speller::AutoSelectPreviousMatch(Context* ctx,
                                      Segment* previous_segment)
{
    if (!auto_select_)
        return false;
    if (ctx->HasMenu())  // if and only if current conversion fails
        return false;
    if (!previous_segment->menu)
        return false;

    size_t start = previous_segment->start;
    size_t end = previous_segment->end;
    std::string input = ctx->input();
    std::string converted = input.substr(0, end);
    if (is_auto_selectable(previous_segment->GetSelectedCandidate(),
                           converted, delimiters_)) {
        // reuse previous match
        ctx->composition()->pop_back();
        ctx->composition()->push_back(std::move(*previous_segment));
        ctx->ConfirmCurrentSelection();
        if (ctx->get_option("_auto_commit")) {
            ctx->set_input(converted);
            ctx->Commit();
            std::string rest = input.substr(end);
            ctx->set_input(rest);
        }
        return true;
    }
    return FindEarlierMatch(ctx, start , end);
}

bool Speller::FindEarlierMatch(Context* ctx, size_t start, size_t end)
{
    if (end <= start + 1)
        return false;
    std::string input = ctx->input();
    std::string converted = input;
    while (--end > start) {
        converted.resize(end);
        ctx->set_input(converted);
        if (!ctx->HasMenu())
            break;
        const Segment& segment(ctx->composition()->back());
        if (is_auto_selectable(segment.GetSelectedCandidate(),
                               converted, delimiters_)) {
            // select previous match
            if (ctx->get_option("_auto_commit")) {
                ctx->Commit();
                std::string rest = input.substr(end);
                ctx->set_input(rest);
                end = 0;
            } else {
                ctx->ConfirmCurrentSelection();
                ctx->set_input(input);
            }
            if (!ctx->HasMenu()) {
                size_t next_start = ctx->composition()->GetCurrentStartPosition();
                size_t next_end = ctx->composition()->GetCurrentEndPosition();
                if (next_start == end) {
                    // continue splitting
                    FindEarlierMatch(ctx, next_start, next_end);
                }
            }
            return true;
        }
    }
    ctx->set_input(input);
    return false;
}


//探测非initials max+1 code补丁
bool Speller::MaxNextCode(Context* ctx,
                          Segment* previous_segment)
{
    if (!auto_select_)
        return false;
    if (!check_max_next_code_)
        return false;

    if (!auto_select_maxcode_excludepattern_.empty() && boost::regex_match(ctx->input(), boost::regex(auto_select_maxcode_excludepattern_)))
        return false;

    const std::string& input(ctx->input());


    if ( input.size() == max_code_length_ + 1 ) {
        ctx->PopInput();
        const Segment& seg(ctx->composition()->back());
        if (is_auto_selectable(seg.GetSelectedCandidate(), ctx->input(), delimiters_)) {
            ctx->ConfirmCurrentSelection();
        }
        return true;
    } else {
        return false;
    }
}
//end

}  // namespace rime





