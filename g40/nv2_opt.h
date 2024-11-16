/*

    C++ command line option parser

    Visit https://github.com/g40

    Copyright (c) Jerry Evans, 1999-2024

    All rights reserved.

    The MIT License (MIT)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

*/

#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <g40/nv2_util.h>

namespace nv2
{
    //-----------------------------------------------------------------------------
    namespace ap
    {
        static
            string_t
            pop(std::vector<string_t>& v)
        {
            string_t r;
            if (v.empty())
                return r;
            r = v.front();
            v.erase(v.begin());
            return r;
        }

        struct Opt
        {
            // the switch string
            string_t swText;
            // help
            string_t helpText;

            enum Tag {
                eUnk = 0,
                eBool,
                eInt,
                eFloat,
                eString,
            };

            union Value {
                void* pv = nullptr;
                bool* pb;
                int* pi;
                float* pf;
                string_t* ps;
            };

            // which element?
            Tag tag = eUnk;
            // pointer to appropriate storage
            Value v;
            //
            Opt() {}
            //
            Opt(const char_t* ps,   //
                bool& arg,          // storage
                const char_t* ph = nullptr) 
            {
                swText = ps;
                tag = eBool;
                v.pb = &arg;
                helpText = (ph ? ph : _T(""));
            }
            //
            Opt(const char_t* ps, int& arg, const char_t* ph = nullptr) 
            {
                swText = ps;
                tag = eInt;
                v.pi = &arg;
                helpText = (ph ? ph : _T(""));
            }
            //
            Opt(const char_t* ps, float& arg, const char_t* ph = nullptr) {
                swText = ps;
                tag = eFloat;
                v.pf = &arg;
                helpText = (ph ? ph : _T(""));
            }
            //
            Opt(const char_t* ps, string_t& arg, const char_t* ph = nullptr) {
                swText = ps;
                tag = eString;
                v.ps = &arg;
                helpText = (ph ? ph : _T(""));
            }
            //
            bool unary() const {
                return (tag == eBool);
            }
            //
            Opt& operator=(const bool& arg)
            {
                if (tag == eBool) {
                    *v.pb = arg;
                }
                return (*this);
            }
            Opt& operator=(const int& arg)
            {
                if (tag == eInt) {
                    *v.pi = arg;
                }
                return (*this);
            }
            Opt& operator=(const float& arg)
            {
                if (tag == eFloat) {
                    *v.pf = arg;
                }
                return (*this);
            }
            // general purpose.
            Opt& operator=(const string_t& arg)
            {
                if (tag == eString) {
                    *v.ps = arg;
                }
                else if (tag == eInt) {
                    *v.pi = std::stoi(arg);
                }
                else if (tag == eFloat) {
                    *v.pf = std::stof(arg);
                }
                else if (tag == eBool) {
                    *v.pb = (arg == _T("true"));
                }
                    return (*this);
            }
            //
            bool operator<(const Opt& arg) const {
                return (swText < arg.swText);
            }
            // general purpose.
            string_t to_string()
            {
                string_t ret;
                if (tag == eString) {
                    ret = (v.ps ? *v.ps : _T("null"));
                }
                else if (tag == eInt) {
                    ret = nv2::n2t(std::to_string(v.pi ? *v.pi : 0));
                }
                else if (tag == eFloat) {
                    ret = nv2::n2t(std::to_string(v.pf ? *v.pf : 0.f));
                }
                else if (tag == eBool) {
                    ret = (v.pb ? (*v.pb ? _T("true") : _T("false")) : _T("false"));
                }
                return ret;
            }
        };

        //
        using iter_t = std::map<string_t,Opt>::iterator;

        // 
        static
            string_t
            to_string(std::vector<Opt> opts, string_t app = _T(""))
        {
            string_t ret = _T("");
            if (app.size()) {
                ret += _T("\n");
                ret += _T("\t");
                ret += app;
                ret += _T("\n\n");
            }
                
            for (auto& opt : opts) {
                ret += _T("\t");
                ret += opt.swText;
                ret += _T(": ");
                ret += opt.helpText;
                ret += _T(" (");
                ret += opt.to_string();
                ret += _T(")\n");
            }
            return ret;
        }

        // return empty on error or should it throw?
        static
            std::vector<string_t>
            parse(int /*argc*/, const char_t* argv[], std::vector<Opt> opts)
        {
            std::map<string_t, Opt> m_opts;
            for (auto& opt : opts) {
                iter_t it = m_opts.find(opt.swText);
                nv2::throw_if(it != m_opts.end(),nv2::acc("Duplicate command line switch was defined:") << opt.swText);
                m_opts[opt.swText] = opt;
            }
            // internal
            std::vector<string_t> args;
            for (int i = 1; argv[i] != 0; i++) {
                args.push_back(argv[i]);
            }
            // positionals
            std::vector<string_t> ret;
            //
            while (!args.empty())
            {
                string_t arg = pop(args);
                if (arg[0] == _T('-'))
                {
                    iter_t it = m_opts.find(arg);
                    nv2::throw_if(it == m_opts.end(), nv2::acc("Unknown switch:") << arg);

                    if (it->second.unary())
                        it->second = true;
                    else {
                        nv2::throw_if(args.empty(), 
                            nv2::acc("Expecting a value for switch") << arg << ":" << it->second.helpText);
                        arg = pop(args);
                        it->second = arg;
                    }
                    continue;
                }
                else if (arg.size()) {
                    ret.push_back(arg);
                }
            }
            //
            return ret;
        }
    }

    //-----------------------------------------------------------------------------
    namespace ap
    {
        static
            void
            test()
        {
            // 
            const char_t* argv[] = {
                _T("executable"),   
                _T("-h"),
                _T("--verbose"),
                _T("--packet-size"),
                _T("1000"),
                _T("abc"),
                _T("def"),
                nullptr,
            };

            int argc = (sizeof(argv) / sizeof(argv[0]));

            // set up defaults
            bool help = false;
            bool verbose = false;
            int packetSize = 16;
            string_t ipPort = _T("CNCA0");
            string_t opPort = _T("CNCB0");

            std::vector<nv2::ap::Opt> opts = {
                { _T("-h"), help, _T("Display help text")},
                { _T("--verbose"), verbose, _T("Display help text") },
                { _T("--packet-size"), packetSize, _T("Set the packet size") },
                { _T("-i"), ipPort, _T("Input port")},
                { _T("-o"), opPort, _T("Output port")}
            };

            // 
            std::vector<string_t> vp = nv2::ap::parse(argc, argv, opts);
            for (auto& p : vp) {
                DBMSG("positional:" << p);
            }

            string_t s = nv2::ap::to_string(opts,_T("u::ap::test"));
        }
    }
}
