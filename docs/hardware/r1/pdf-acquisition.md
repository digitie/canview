# 제조사 PDF 확보 기록

2026-09-05, `F:/dev/canview`에서 누락된 5개 문서를 약 10분 동안 조사했다. 3개를 확보했고 2개는 미확보다. `embedded-documentation`의 근거 추적 원칙에 따라 공식 출처, 실제 다운로드 경로와 개정을 구분했다. 아래 파일명은 `hardware/references/pdf/` 기준이다.

| 파일 | 공식 URL | 실제 다운로드 URL 또는 실패한 경로 | 상태·개정·쪽수 |
|---|---|---|---|
| `stm32g4-hardware.pdf` | [ST AN5093](https://www.st.com/resource/en/application_note/an5093-getting-started-with-stm32g4-series--hardware-development-boards-stmicroelectronics.pdf) | [ST 중국 서버](https://www.st.com.cn/resource/en/application_note/dm00442716-getting-started-with-stm32g4-series-hardware-development-boards-stmicroelectronics.pdf), [device.report 사본](https://device.report/m/54eb2ab896aa637ca94c1284937681a030e804720161267247ae310baa726648.pdf) | **미확보**. ST 서버는 10~15초 읽기 시간 초과, 미러는 HTTP 403. 웹 열람에서는 AN5093 Rev 2, 2019-10, 35쪽 확인. 파일 생성 안 함. |
| `littelfuse-451.pdf` | [Littelfuse 451/453](https://www.littelfuse.com/assetdocs/fuse-451-and-453-datasheet?assetguid=533cd5cc-956c-4243-867f-6ab5a62f6ba1) | [Megastar 제조사 문서 사본](https://www.megastar.com/content/pdfs/451-453%20Series.pdf) | **구판 확보**, HTTP 200. `Revised: GD. 09/18/23`, 4쪽. 공식 사이트의 `12/01/25` 개정과 다름. |
| `max20040.pdf` | [ADI MAX20039/MAX20040](https://www.analog.com/media/en/technical-documentation/data-sheets/max20039-max20040.pdf) | [LCSC 제조사 문서 사본](https://atta.szlcsc.com/upload/public/pdf/source/20240816/11E54D3F66F8CB0476CD7D650874A4B7.pdf) | **구판 확보**, HTTP 200. `19-100271; Rev 15; 3/24`, 20쪽. 공식 사이트의 Rev 16, 4/25와 다름. |
| `max20040-evm.pdf` | [ADI MAX20040 평가보드 문서](https://www.analog.com/media/en/technical-documentation/data-sheets/max20040evkit.pdf) | [Farnell 제조사 문서 사본](https://www.farnell.com/datasheets/4563848.pdf) | **확보**, HTTP 200. `319-100214; Rev 3; 3/24`, 8쪽. 공식 웹 열람의 개정·쪽수와 일치. |
| `max20040-schematic.pdf` | [ADI 단독 회로도](https://www.analog.com/media/en/technical-documentation/eval-board-schematic/max20039_40_evkit_schematic.pdf) | 공식 URL에서 Windows 다운로드 및 WSL `curl --http1.1 --ipv4` 시도 | **미확보**. 12~15초 동안 응답 본문 0바이트로 시간 초과. 웹 열람에서는 1쪽 확인, 로컬 개정 검증 불가. 파일 생성 안 함. |

Windows native Python과 `pypdf.PdfReader(strict=True)`로 확보한 3개 파일의 `%PDF` 시작, `%%EOF` 종료, 전체 페이지 파싱, 문서 개정과 본문을 확인했다. Littelfuse의 전기 규격 2쪽·패드 도면 4쪽, MAX의 핀 설명 9~10쪽·개정 이력 20쪽, 평가보드의 BOM 3쪽·회로도 4쪽·개정 이력 8쪽을 확인했다. 추출 본문에서 배포사 추가 문구는 발견하지 못했다. 다운로드 응답 바이트를 재생성·페이지 추출·수정 없이 저장했다.

| 파일 | 바이트 수 | SHA-256 |
|---|---:|---|
| `littelfuse-451.pdf` | 512218 | `80da7135f5510b8e801e4c1015634e5cc9593a132c16d5b33cfed0d0900f6158` |
| `max20040.pdf` | 1475432 | `b2daf874befab4f8aff4716f7030696aee484339616bd9e6dcca459d432b213b` |
| `max20040-evm.pdf` | 473931 | `0807e00bbf34b2ecf8fae2fb70d2585f45d3c0f8087315cefa1f14cc5966c826` |

검증 한계: 공식 서버에서 원본 바이트를 받지 못했으므로 미러와 공식 파일의 SHA-256 동일성은 미검증이다. 제조사 표제·개정·문서 구조를 확인한 사본이며, 최신 개정과 동등하다고 판정하지 않는다. MAX Rev 16의 공식 개정 이력에는 절대최대정격의 Note 1·2 수정이 있으므로 Rev 15를 최신 정격 근거로 간주하면 안 된다. Littelfuse 최신 개정과의 전체 차이도 미검증이다.

배제한 경로: Mouser PDF 링크 여러 개는 HTTP 200이지만 HTML 본문을 반환했다. Arrow 평가보드 사본은 웹 열람에서 `Downloaded from Arrow.com.` 문구가 확인되어 제외했다. ST 일본·중국 문서 포털은 다시 ST 공식 URL을 가리켰다. 단독 회로도가 없다는 이유로 평가보드 PDF의 4쪽을 추출하거나 대체 PDF를 만들지 않았다.

이 작업은 위 신규 PDF 3개와 이 기록만 작성했다. 기존 PDF, `hardware/references/manifest.json`, `tools/hardware/references.json` 및 다른 작업자의 변경은 수정하지 않았고 Git stage·commit·push도 수행하지 않았다. 주 작업자는 구판 2개의 수용 여부를 판단한 뒤 실제 개정·다운로드 URL·해시를 참조 목록에 반영해야 한다.

## 제한 시간 추가 시도

2026-09-05 12:05~12:13 KST, AN5093 Rev 2와 MAX Rev 16 원문만 추가 조사했다. **신규 확보 없음**으로 10분 이내 종료했다.

- AN5093: 위 ST 공식 URL에 `?download=1`, `?_=20260905`를 붙이거나 중국 도메인·문서 번호 경로(`en.DM00442716.pdf`, `dm00442716.pdf`)를 사용해도 Windows 다운로드는 약 10~14초 읽기 시간 초과였다. 위 `device.report` 원문 경로에 `?download=1`을 붙인 요청과 [与非网 공개 다운로드 경로](https://www.eefocus.com/file/outside/download?post_id=1511115)는 HTTP 403이었다.
- MAX Rev 16: 위 ADI 공식 URL의 동일 쿼리 변형·대소문자 변형·루트 도메인과 [Maxim 레거시 경로](https://datasheets.maximintegrated.com/en/ds/MAX20039-MAX20040.pdf), [Maxim PDF 서버](https://pdfserv.maximintegrated.com/en/ds/MAX20039-MAX20040.pdf)도 약 10~14초 읽기 시간 초과였다. Farnell이 연결한 [IHS 제조사 문서 사본](https://4donline.ihs.com/images/VipMasterIC/IC/ANDI/ANDI-S-A0018986318/ANDI-S-A0018986596-1.pdf?hkey=6D3A4C79FDBF58556ACFDE234799DDF0)은 웹 열람에서 Rev 14, 9/22, 20쪽으로 확인되어 저장하지 않았다. Mouser의 쿼리·지역 도메인 변형은 계속 HTTP 200 HTML을 반환했다.
- `stm32g4-hardware.pdf`와 `max20040-rev16.pdf`는 생성하지 않았다. 기존 3개 PDF의 SHA-256을 Windows에서 재확인해 위 기록과 일치함을 확인했다. 구판 덮어쓰기·대체 PDF 생성·참조 JSON 수정은 없었으며, 다른 에이전트가 맡은 패키지 도면 조사는 중복 수행하지 않았다.
