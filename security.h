/*
 * Copyright (c) 2015 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __SECURITY_H__
#define __SECURITY_H__
 
#include <inttypes.h>
 
#define MBED_DOMAIN "68763e6f-5c5a-48f2-9fac-fe52d46a134e"
#define MBED_ENDPOINT_NAME "f53c0927-3c18-4cc5-a356-a086f5a271e6"
 
const uint8_t SERVER_CERT[] = "-----BEGIN CERTIFICATE-----\r\n"
"MIIBmDCCAT6gAwIBAgIEVUCA0jAKBggqhkjOPQQDAjBLMQswCQYDVQQGEwJGSTEN\r\n"
"MAsGA1UEBwwET3VsdTEMMAoGA1UECgwDQVJNMQwwCgYDVQQLDANJb1QxETAPBgNV\r\n"
"BAMMCEFSTSBtYmVkMB4XDTE1MDQyOTA2NTc0OFoXDTE4MDQyOTA2NTc0OFowSzEL\r\n"
"MAkGA1UEBhMCRkkxDTALBgNVBAcMBE91bHUxDDAKBgNVBAoMA0FSTTEMMAoGA1UE\r\n"
"CwwDSW9UMREwDwYDVQQDDAhBUk0gbWJlZDBZMBMGByqGSM49AgEGCCqGSM49AwEH\r\n"
"A0IABLuAyLSk0mA3awgFR5mw2RHth47tRUO44q/RdzFZnLsAsd18Esxd5LCpcT9w\r\n"
"0tvNfBv4xJxGw0wcYrPDDb8/rjujEDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0E\r\n"
"AwIDSAAwRQIhAPAonEAkwixlJiyYRQQWpXtkMZax+VlEiS201BG0PpAzAiBh2RsD\r\n"
"NxLKWwf4O7D6JasGBYf9+ZLwl0iaRjTjytO+Kw==\r\n"
"-----END CERTIFICATE-----\r\n";
 
const uint8_t CERT[] = "-----BEGIN CERTIFICATE-----\r\n"
"MIIB0DCCAXOgAwIBAgIERR4vGDAMBggqhkjOPQQDAgUAMDkxCzAJBgNVBAYTAkZ\r\n"
"JMQwwCgYDVQQKDANBUk0xHDAaBgNVBAMME21iZWQtY29ubmVjdG9yLTIwMTgwHh\r\n"
"cNMTcwMzE1MDYwNDIzWhcNMTgxMjMxMDYwMDAwWjCBoTFSMFAGA1UEAxNJNjg3N\r\n"
"jNlNmYtNWM1YS00OGYyLTlmYWMtZmU1MmQ0NmExMzRlL2Y1M2MwOTI3LTNjMTgt\r\n"
"NGNjNS1hMzU2LWEwODZmNWEyNzFlNjEMMAoGA1UECxMDQVJNMRIwEAYDVQQKEwl\r\n"
"tYmVkIHVzZXIxDTALBgNVBAcTBE91bHUxDTALBgNVBAgTBE91bHUxCzAJBgNVBA\r\n"
"YTAkZJMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEh1JGp18b2s3d0UH+AJZuZ\r\n"
"enOuI4qfK3FI9kO0nwyGr1vvi1Gbj9KFI+hW+U2SIITj6RPlsELxLla/cYwRiTD\r\n"
"ozAMBggqhkjOPQQDAgUAA0kAMEYCIQDo12JJyRHLz4bPmopsd8wqYqM5bJbvCzF\r\n"
"2bJ5hZygHnAIhAI7PRYZLt+cLMlDZtK1QCHrAvYEtIuOykdxleTSdUsev\r\n"
"-----END CERTIFICATE-----\r\n";
 
const uint8_t KEY[] = "-----BEGIN PRIVATE KEY-----\r\n"
"MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgEwL/+ur+mjm40FTr\r\n"
"w70+/5eJKWDRANOMt0u/bFtdhUehRANCAASHUkanXxvazd3RQf4Alm5l6c64jip8\r\n"
"rcUj2Q7SfDIavW++LUZuP0oUj6Fb5TZIghOPpE+WwQvEuVr9xjBGJMOj\r\n"
"-----END PRIVATE KEY-----\r\n";
 
#endif //__SECURITY_H__