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
#define MBED_ENDPOINT_NAME "a553c58c-2f84-458e-8a43-bf0e0bf9be06"
 
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
"MIIBzjCCAXOgAwIBAgIEMML2lzAMBggqhkjOPQQDAgUAMDkxCzAJBgNVBAYTAkZ\r\n"
"JMQwwCgYDVQQKDANBUk0xHDAaBgNVBAMME21iZWQtY29ubmVjdG9yLTIwMTgwHh\r\n"
"cNMTcwNDI0MDYwMjQ0WhcNMTgxMjMxMDYwMDAwWjCBoTFSMFAGA1UEAxNJNjg3N\r\n"
"jNlNmYtNWM1YS00OGYyLTlmYWMtZmU1MmQ0NmExMzRlL2E1NTNjNThjLTJmODQt\r\n"
"NDU4ZS04YTQzLWJmMGUwYmY5YmUwNjEMMAoGA1UECxMDQVJNMRIwEAYDVQQKEwl\r\n"
"tYmVkIHVzZXIxDTALBgNVBAcTBE91bHUxDTALBgNVBAgTBE91bHUxCzAJBgNVBA\r\n"
"YTAkZJMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEg8iKCKQjWY4QabbcseKWz\r\n"
"dQ2jhuJCUSJ9izq4b0OXZNTlJWgcqZTutgWle1PmLveftxexgHolb1U9JOeANYD\r\n"
"QzAMBggqhkjOPQQDAgUAA0cAMEQCIH9r+5mNOx/QF4B6R3Mnjt7vbzJit+0GRa0\r\n"
"SdXlKvvZ1AiA+TPtmYphpIivohvgo6ZPgl+GbWLDcokQxM+kzzCwAmA==\r\n"
"-----END CERTIFICATE-----\r\n";
 
const uint8_t KEY[] = "-----BEGIN PRIVATE KEY-----\r\n"
"MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgfVriOA6WODgd9heK\r\n"
"bJBCoYsC4HZstxb5oCh+MdFEUNGhRANCAASDyIoIpCNZjhBpttyx4pbN1DaOG4kJ\r\n"
"RIn2LOrhvQ5dk1OUlaByplO62BaV7U+Yu95+3F7GAeiVvVT0k54A1gND\r\n"
"-----END PRIVATE KEY-----\r\n";
 
#endif //__SECURITY_H__