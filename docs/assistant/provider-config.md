# Provider Configuration (Windows)

## Supported Providers

- `openai`
- `anthropic`
- `deepseek`

## Credential Targets

Store API keys in Windows Credential Manager (Generic Credential), for example:

- `assistant/openai`
- `assistant/anthropic`
- `assistant/deepseek`

Never store plaintext keys in repo config or logs.

## Timeout and Quality

- Request timeout is carried in `AiAnalyzeRequest.timeout_ms`
- Provider adapters now include `text`, `context`, `scene`, `timeout_ms` in request body
- Strict small budget can degrade to fallback mode

## Error Semantics

- `401`: missing provider credential
- `408`: timeout or degraded parse-failure fallback
- `0`: success

## Privacy Notes

- Do not print API keys in logs
- Keep payload telemetry anonymized (error code/provider/timing only)
- Prefer in-memory handling over persistent raw-text storage

## Troubleshooting

1. If always `401`, verify Credential Manager target names.
2. If frequent `408`, increase timeout budget or reduce request frequency.
3. If parse fallback occurs often, check provider response schema drift.
