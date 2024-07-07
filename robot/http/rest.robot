*** Settings ***
Documentation     A test suite for examples/http/rest.hpp.
Library           String
Library           Process
Library           RequestsLibrary
Test Timeout      1 minute

Suite Setup       Start Servers
Suite Teardown    Stop Servers

*** Variables ***
${HTTP_URL}             http://localhost:55501
${HTTPS_URL}            https://localhost:55502
${BINARY_PATH}          /path/to/the/server
${SSL_PATH}             /path/to/the/pem/files
${MAX_REQUEST_SIZE}     2048

# Prometheus variables
${PR_SERVER_PORT}       55517
${PR_HTTP_URL}          http://localhost:55517/metrics
${PR_HTTP_BAD_URL}      http://localhost:55517/foobar

${PR_SSL_SERVER_PORT}       55518
${PR_HTTPS_URL}          https://localhost:55518/metrics
${PR_HTTPS_BAD_URL}      https://localhost:55518/foobar

*** Test Cases ***
HTTP Test Add Key Value Pair
    [Tags]    POST
    Add Key Value Pair    ${HTTP_URL}    foo    bar
    Get Key Value Pair    ${HTTP_URL}    foo    bar

HTTP Test Update Key Value Pair
    [Tags]    POST
    Add Key Value Pair       ${HTTP_URL}    foo    bar
    Update Key Value Pair    ${HTTP_URL}    foo    baz
    Get Key Value Pair       ${HTTP_URL}    foo    baz

HTTP Test Delete Key Value Pair
    [Tags]    DELETE
    Add Key Value Pair       ${HTTP_URL}    foo    bar
    Delete Key Value Pair    ${HTTP_URL}    foo
    Key Should Not Exist     ${HTTP_URL}    foo

HTTPS Test Add Key Value Pair
    [Tags]    POST
    Add Key Value Pair    ${HTTPS_URL}    foo    bar
    Get Key Value Pair    ${HTTPS_URL}    foo    bar

HTTPS Test Update Key Value Pair
    [Tags]    POST
    Add Key Value Pair       ${HTTPS_URL}    foo    bar
    Update Key Value Pair    ${HTTPS_URL}    foo    baz
    Get Key Value Pair       ${HTTPS_URL}    foo    baz

HTTPS Test Delete Key Value Pair
    [Tags]    DELETE
    Add Key Value Pair       ${HTTPS_URL}    foo    bar
    Delete Key Value Pair    ${HTTPS_URL}    foo
    Key Should Not Exist     ${HTTPS_URL}    foo

HTTPS Test Request Exceeds Maximum Size
    [Tags]    POST
    ${large_value}=    Generate Random String       ${MAX_REQUEST_SIZE}
    POST    ${HTTP_URL}/api/foo    data=${large_value}    expected_status=413    verify=${False}

HTTP Propetheus Endpoint
    [Tags]    GET
    ${resp}=    GET    ${PR_HTTP_URL}
    ${content}=    Decode Bytes To String    ${resp.content}    utf-8
    Should Contain    ${content}    caf_system_running_actors
    Run Keyword And Expect Error    *    GET    ${PR_HTTP_BAD_URL}

HTTPS Propetheus Endpoint
    [Tags]    GET
    ${resp}=    GET    ${PR_HTTPS_URL}   verify=${False}
    ${content}=    Decode Bytes To String    ${resp.content}    utf-8
    Should Contain    ${content}    caf_system_running_actors
    Run Keyword And Expect Error    *    GET    ${PR_HTTPS_BAD_URL}   verify=${False}

*** Keywords ***
Start Servers
    Start Process  ${BINARY_PATH}  -p  55501  -r  ${MAX_REQUEST_SIZE}  --caf.net.prometheus-http.port  ${PR_SERVER_PORT}
    Start Process  ${BINARY_PATH}  -p  55502  -r  ${MAX_REQUEST_SIZE}  -k  ${SSL_PATH}/key.pem  -c  ${SSL_PATH}/cert.pem
    ...    --caf.net.prometheus-http.port  ${PR_SSL_SERVER_PORT}  --caf.net.prometheus-http.tls.key-file  ${SSL_PATH}/key.pem
    ...    --caf.net.prometheus-http.tls.cert-file  ${SSL_PATH}/cert.pem
    Wait Until Keyword Succeeds    5s    125ms    Check If HTTP Server Is Reachable
    Wait Until Keyword Succeeds    5s    125ms    Check If HTTPS Server Is Reachable

Stop Servers
    Run Keyword And Ignore Error    Terminate All Processes

Check If HTTP Server Is Reachable
    Log         Try reaching ${HTTP_URL}/status.
    ${resp}=    GET    ${HTTP_URL}/status    expected_status=204

Check If HTTPS Server Is Reachable
    Log         Try reaching ${HTTPS_URL}/status.
    ${resp}=    GET    ${HTTPS_URL}/status    expected_status=204    verify=${False}

Add Key Value Pair
    [Arguments]    ${base_url}    ${key}    ${value}
    ${resp}=    POST    ${base_url}/api/${key}    data=${value}    expected_status=204    verify=${False}

Get Key Value Pair
    [Arguments]    ${base_url}    ${key}    ${expected_value}
    ${resp}=    GET     ${base_url}/api/${key}   expected_status=200    verify=${False}
    Should Be Equal As Strings    ${resp.content}    ${expected_value}

Update Key Value Pair
    [Arguments]    ${base_url}    ${key}    ${new_value}
    ${resp}=    POST     ${base_url}/api/${key}    data=${new_value}   expected_status=204    verify=${False}

Delete Key Value Pair
    [Arguments]    ${base_url}    ${key}
    ${resp}=    DELETE    ${base_url}/api/${key}   expected_status=204    verify=${False}

Key Should Not Exist
    [Arguments]    ${base_url}    ${key}
    ${resp}=    GET     ${base_url}/api/${key}   expected_status=404    verify=${False}
