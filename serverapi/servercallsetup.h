/******************************************************************************
 * Copyright (C) 2016 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <QtCore/QUrlQuery>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QByteArray>
#include <QtCore/QMimeType>

class QNetworkReply;

namespace QMatrixClient
{
    namespace ServerApi
    {
        class ApiPath
        {
            public:
                ApiPath(QString shortPath,
                        QString scope = "client",
                        QString version = "r0");
                ApiPath(const char * shortPath)
                    : ApiPath(QString(shortPath))
                { }
                QString toString() const { return fullPath; }

            private:
                QString fullPath;
        };
        class Query : public QUrlQuery
        {
            public:
                using QUrlQuery::QUrlQuery;
                Query() = default;
                explicit Query(std::initializer_list< QPair<QString, QString> > l)
                {
                    setQueryItems(l);
                }
                using QUrlQuery::addQueryItem;
                void addQueryItem(const QString& name, int value)
                {
                    addQueryItem(name, QString::number(value));
                }
        };
        class Data : public QJsonObject
        {
            public:
                Data() = default;
                explicit Data(std::initializer_list< QPair<QString, QJsonValue> > l)
                {
                    for (auto i: l)
                        insert(i.first, i.second);
                }
                using QJsonObject::insert;
                void insert(const QString& name, const QStringList& sl);
                template <typename T>
                void insert(const QString& name, const QVector<T>& vv)
                {
                    QJsonArray ja;
                    std::copy(vv.begin(), vv.end(), std::back_inserter(ja));
                    insert(name, ja);
                }

                QByteArray dump() const;
        };
        enum class HttpVerb { Get, Put, Post, Delete };

        class RequestConfig
        {
            public:
                RequestConfig(QString name, HttpVerb type, ApiPath endpoint,
                              QMimeType contentType, QByteArray data,
                              Query query = Query(), bool needsToken = true)
                    : _name(name), _type(type), _endpoint(endpoint)
                    , _query(query), _contentType(contentType), _data(data)
                    , _needsToken(needsToken)
                { }

                RequestConfig(QString name, HttpVerb type, ApiPath endpoint,
                              Query query, Data data = Data(),
                              bool needsToken = true)
                    : RequestConfig(name, type, endpoint,
                        JsonMimeType, data.dump(), query, needsToken)
                { }

                RequestConfig(QString name, HttpVerb type, ApiPath endpoint,
                              bool needsToken = true)
                    : RequestConfig(name, type, endpoint,
                                    Query(), Data(), needsToken)
                { }

                QString name() const { return _name; }
                HttpVerb type() const { return _type; }
                QString apiPath() const { return _endpoint.toString(); }
                QUrlQuery query() const { return _query; }
                void setQuery(const Query& q) { _query = q; }
                QMimeType contentType() const { return _contentType; }
                QByteArray data() const { return _data; }
                void setData(const Data& json) { _data = json.dump(); }
                bool needsToken() const { return _needsToken; }

            protected:
                const QString _name;
                const HttpVerb _type;
                ApiPath _endpoint;
                Query _query;
                QMimeType _contentType;
                QByteArray _data;
                bool _needsToken;

                static const QMimeType JsonMimeType;
        };

        enum StatusCode { NoError = 0 // To be compatible with Qt conventions
            , InfoLevel = 0 // Information codes below
            , Success = 0
            , PendingResult = 1
            , WarningLevel = 50 // Warnings below
            , ErrorLevel = 100 // Errors below
            , NetworkError = 100
            , JsonParseError
            , TimeoutError
            , ContentAccessError
            , UserDefinedError = 200
        };

        /**
         * This structure stores status of a server call. The status consists
         * of a code, that is described (but not delimited) by the respective enum,
         * and a freeform message.
         *
         * To extend the list of error codes, define an (anonymous) enum
         * with additional values starting at CallStatus::UserDefinedError
         *
         * @see Call, CallSetup
         */
        class Status
        {
            public:
                Status(StatusCode c) : code(c) { }
                Status(int c, QString m) : code(c), message(m) { }

                bool good() const { return code < WarningLevel; }

                int code;
                QString message;
        };

        /**
         * This class encapsulates a return value from a result handling function
         * (either an adaptor, or makeResult) packed together with CallStatus.
         * This allows to return error codes and messages without using exceptions.
         * A notable case is Result<void>, which represents a result with no value,
         * only with a status; but provides respective interface.
         */
        template <typename T>
        class Result
        {
            public:
                using cref_type = const T&;

                Result(const Status& cs) : m_status(cs) { }
                Result(int code, QString message) : m_status(code, message) { }
                Result(const T& value, const Status& cs = Success)
                    : m_status(cs), m_value(value)
                { }
                Result(T&& value, const Status& cs = Success)
                    : m_status(cs), m_value(std::move(value))
                { }

                Status status() const { return m_status; }
                cref_type get() const { return m_value; }

                template <typename HandlerT>
                void passTo(HandlerT handler) const { handler(get()); }

            private:
                Status m_status;
                T m_value;
        };

        template <>
        class Result<void>
        {
            public:
                using cref_type = void;

                Result(const Status& cs) : m_status(cs) { }
                Result(int code, QString message) : m_status(code, message) { }

                Status status() const { return m_status; }

                template <typename HandlerT>
                void passTo(HandlerT handler) const { handler(); }

            private:
                Status m_status;
        };

        /**
         * @brief A base class for a call configuration
         *
         * Derive from this class to make a call configuration that can be used
         * with PendingCall<>. Derived classes should:
         * - Provide a constructor or constructors that will initialize
         *   RequestConfig fields inside the base class;
         * - (optionally) Provide a function or a function object with the same
         *   signature as checkReply() below to override the network reply
         *   preliminary checking.
         * - (optionally) Provide a function or a function object with the name
         *   parseReply that accepts either QNetworkReply* or QJsonObject
         *   and returns a data structure (optionally enclosed into Result<>)
         *   parsed from the reply or from the respective JSON. Only one overload
         *   should exist; having both QNetworkReply* and QJsonObject versions
         *   will cause a compilation error. The default implementation blindly
         *   returns Success without analysing the reply.
         *
         * @see RequestConfig, Call, PendingCall
         */
        class CallConfig : public RequestConfig
        {
            public:
                using RequestConfig::RequestConfig;

                Status checkReply(const QNetworkReply* reply) const;
                Result<QJsonObject> readAsJson(QNetworkReply* reply) const;
        };

        class CallConfigNoReplyBody : public CallConfig
        {
            public:
                using CallConfig::CallConfig;

                Status parseReply(QNetworkReply*) const
                {
                    return { Success };
                }
        };
    }
}