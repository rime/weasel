# Windows IME Assistant Usage

## Scope

Send Quality Assistant is a Windows-side helper for chat send-intent flows.
It should improve text quality before send, but must never block normal send.

## Trigger (Send Intent)

- Enter key passes through IME to app after recent commit
- Typical chat apps: WeChat / QQ / TIM / DingTalk / Feishu / Lark / Slack / Telegram
- Non-send editing actions should not trigger assistant panel

## Panel Actions

- `Apply`: apply selected suggestion text
- `Replace All`: replace entire draft with suggested rewrite
- `Ignore`: close panel and keep original text/send flow

## Fallback Rules

- Assistant timeout/error must not block send path
- Parse failure degrades to risk-only fallback
- Missing credential degrades to no-op/risk-only (by server policy)

## Current Limitations

- Provider HTTP transport is not fully integrated yet in this branch
- Suggestion panel is functional scaffold and still needs richer layout/polish
- Some host apps may not provide stable send-intent signals

## Debug Checklist

- Verify `assistant_enabled=1` in config response
- Verify `ai_analyze.*` payload is returned on send intent
- Verify `Ignore` always returns control to normal send
- Verify failures still keep input/send responsive
