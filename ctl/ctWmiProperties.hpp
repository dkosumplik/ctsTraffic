/*

Copyright (c) Microsoft Corporation
All rights reserved.

Licensed under the Apache License, Version 2.0 (the ""License""); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.

See the Apache Version 2.0 License for specific language governing permissions and limitations under the License.

*/

#pragma once

// cpp headers
#include <iterator>
#include <exception>
#include <stdexcept>
// os headers
#include <windows.h>
#include <Wbemidl.h>
// local headers
#include "ctComInitialize.hpp"
#include "ctWmiException.hpp"
#include "ctWmiService.hpp"
#include "ctVersionConversion.hpp"


namespace ctl {

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    ///
    /// class ctWmiProperties
    ///
    /// Exposes enumerating properties of a WMI Provider through an iterator interface.
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    class ctWmiProperties
    {
    private:
        ctWmiService wbemServices;
        ctComPtr<IWbemClassObject> wbemClass;

    public:
        //
        // forward declare iterator class
        //
        class iterator;


        ctWmiProperties(_In_ const ctWmiService& _wbemServices, _In_ const ctComPtr<IWbemClassObject>& _wbemClass) : 
            wbemServices(_wbemServices), wbemClass(_wbemClass)
        {
        }

        ctWmiProperties(_In_ const ctWmiService& _wbemServices, _In_ LPCWSTR _className) : 
            wbemServices(_wbemServices), wbemClass()
        {
            HRESULT hr = this->wbemServices->GetObjectW(
                ctComBstr(_className).get(),
                0,
                nullptr,
                this->wbemClass.get_addr_of(),
                nullptr);
            if (FAILED(hr)) {
                throw ctWmiException(hr, L"IWbemServices::GetObject", L"ctWmiProperties::ctWmiProperties", false);
            }
        }

        ctWmiProperties(_In_ const ctWmiService& _wbemServices, _In_ const ctComBstr& _className)
            : wbemServices(_wbemServices), wbemClass()
        {
            HRESULT hr = this->wbemServices->GetObjectW(
                _className.get(),
                0,
                nullptr,
                this->wbemClass.get_addr_of(),
                nullptr);
            if (FAILED(hr)) {
                throw ctWmiException(hr, L"IWbemServices::GetObject", L"ctWmiProperties::ctWmiProperties", false);
            }
        }


        ////////////////////////////////////////////////////////////////////////////////
        ///
        /// begin() and end() to return property iterators
        ///
        /// begin() arguments:
        ///   _wbemServices: a instance of IWbemServices
        ///   _className: a string of the class name which to enumerate
        ///   _fNonSystemPropertiesOnly: flag to control if should only enumerate
        ///       non-system properties
        ///
        /// begin() will throw an exception derived from std::exception on error
        /// end() is no-fail / no-throw
        ///
        ////////////////////////////////////////////////////////////////////////////////
        iterator begin(_In_ bool _fNonSystemPropertiesOnly = true)
        {
            return iterator(this->wbemClass, _fNonSystemPropertiesOnly);
        }
        iterator end() NOEXCEPT
        {
            return iterator();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        ///
        /// class ctWmiProperties::iterator
        ///
        /// A forward iterator class type to enable forward-traversing instances of the queried
        ///  WMI provider
        ///
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        class iterator
        {
        private:
            static const unsigned long END_ITERATOR_INDEX = 0xffffffff;

            ctComPtr<IWbemClassObject> wbemClassObj;
            ctComBstr propName;
            CIMTYPE   propType;
            DWORD     dwIndex;

        public:
            ////////////////////////////////////////////////////////////////////////////////
            ///
            /// c'tor and d'tor
            /// - default c'tor is an 'end' iterator
            /// - traversal requires the callers IWbemServices interface and class name
            ///
            ////////////////////////////////////////////////////////////////////////////////
            iterator() NOEXCEPT : 
                wbemClassObj(),
                propName(),
                propType(0),
                dwIndex(END_ITERATOR_INDEX)
            {
            }
            iterator(_In_ const ctComPtr<IWbemClassObject>& _classObj, _In_ bool _fNonSystemPropertiesOnly) : 
                dwIndex(0),
                wbemClassObj(_classObj),
                propName(),
                propType(0)
            {
                HRESULT hr = wbemClassObj->BeginEnumeration((_fNonSystemPropertiesOnly) ? WBEM_FLAG_NONSYSTEM_ONLY : 0);
                if (FAILED(hr)) {
                    throw ctWmiException(hr, wbemClassObj.get(), L"IWbemClassObject::BeginEnumeration", L"ctWmiProperties::iterator::iterator", false);
                }

                increment();
            }

            ~iterator() NOEXCEPT
            {
            }

            ////////////////////////////////////////////////////////////////////////////////
            ///
            /// copy c'tor and copy assignment
            ///
            /// Note that both are no-fail/no-throw operations
            ///
            ////////////////////////////////////////////////////////////////////////////////
            iterator(_In_ const iterator& _i) NOEXCEPT : 
                dwIndex(_i.dwIndex),
                wbemClassObj(_i.wbemClassObj),
                propName(_i.propName),
                propType(_i.propType)
            {
            }
            iterator& operator =(_In_ const iterator& _i) NOEXCEPT
            {
                iterator copy(_i);
                this->swap(copy);
                return *this;
            }

            void swap(_Inout_ iterator& _i) NOEXCEPT
            {
                using std::swap;
                swap(this->dwIndex, _i.dwIndex);
                swap(this->wbemClassObj, _i.wbemClassObj);
                swap(this->propName, _i.propName);
                swap(this->propType, _i.propType);
            }

            ////////////////////////////////////////////////////////////////////////////////
            ///
            /// accessors:
            /// - dereference operators to access the property name
            /// - explicit type() method to expose its CIM type
            ///
            ////////////////////////////////////////////////////////////////////////////////
            ctComBstr& operator*()
            {
                if (this->dwIndex == this->END_ITERATOR_INDEX) {
                    throw std::out_of_range("ctWmiProperties::iterator::operator - invalid subscript");
                }
                return this->propName;
            }
            ctComBstr* operator->()
            {
                if (this->dwIndex == this->END_ITERATOR_INDEX) {
                    throw std::out_of_range("ctWmiProperties::iterator::operator-> - invalid subscript");
                }
                return &(this->propName);
            }
            CIMTYPE type()
            {
                if (this->dwIndex == this->END_ITERATOR_INDEX) {
                    throw std::out_of_range("ctWmiProperties::iterator::type - invalid subscript");
                }
                return this->propType;
            }

            ////////////////////////////////////////////////////////////////////////////////
            ///
            /// comparison and arithmatic operators
            /// 
            /// comparison operators are no-throw/no-fail
            /// arithmatic operators can fail 
            /// - throwing a ctWmiException object capturing the WMI failures
            ///
            ////////////////////////////////////////////////////////////////////////////////
            bool operator==(_In_ const iterator& _iter) const NOEXCEPT
            {
                if (this->dwIndex != this->END_ITERATOR_INDEX) {
                    return ((this->dwIndex == _iter.dwIndex) &&
                            (this->wbemClassObj == _iter.wbemClassObj));
                } else {
                    return (this->dwIndex == _iter.dwIndex);
                }
            }
            bool operator!=(_In_ const iterator& _iter) const NOEXCEPT
            {
                return !(*this == _iter);
            }

            // preincrement
            iterator& operator++()
            {
                this->increment();
                return *this;
            }
            // postincrement
            iterator operator++(_In_ int)
            {
                iterator temp(*this);
                this->increment();
                return temp;
            }
            // increment by integer
            iterator& operator+=(_In_ DWORD _inc)
            {
                for (unsigned loop = 0; loop < _inc; ++loop) {
                    this->increment();
                    if (this->dwIndex == this->END_ITERATOR_INDEX) {
                        throw std::out_of_range("ctWmiProperties::iterator::operator+= - invalid subscript");
                    }
                }
                return *this;
            }

            ////////////////////////////////////////////////////////////////////////////////
            ///
            /// iterator_traits
            /// - allows <algorithm> functions to be used
            ///
            ////////////////////////////////////////////////////////////////////////////////
            typedef std::forward_iterator_tag  iterator_category;
            typedef ctComBstr                  value_type;
            typedef int                        difference_type;
            typedef ctComBstr*                 pointer;
            typedef ctComBstr&                 reference;


        private:
            void increment()
            {
                if (dwIndex == END_ITERATOR_INDEX) {
                    throw std::out_of_range("ctWmiProperties::iterator - cannot increment: at the end");
                }

                CIMTYPE next_cimtype;
                ctComBstr next_name;
                ctComVariant next_value;
                HRESULT hr = this->wbemClassObj->Next(
                    0,
                    next_name.get_addr_of(),
                    next_value.get(),
                    &next_cimtype,
                    nullptr);

                switch (hr) {
                    case WBEM_S_NO_ERROR: {
                        // update the instance members
                        ++dwIndex;
                        using std::swap;
                        swap(propName, next_name);
                        swap(propType, next_cimtype);
                        break;
                    }

#pragma warning (suppress : 6221)
                    // "Implicit cast between semantically different integer types:  comparing HRESULT to an integer.  Consider using SUCCEEDED or FAILED macros instead."
                    // Directly comparing the HRESULT return to WBEM_S_NO_ERROR, even though WBEM did not properly define that constant as an HRESULT
                    case WBEM_S_NO_MORE_DATA: {
                        // at the end...
                        dwIndex = END_ITERATOR_INDEX;
                        propName.reset();
                        propType = 0;
                        break;
                    }

                    default:
                        throw ctWmiException(
                            hr, 
                            this->wbemClassObj.get(), 
                            L"IEnumWbemClassObject::Next", 
                            L"ctWmiProperties::iterator::increment", 
                            false);
                }
            }
        };
    };

} // namespace ctl
