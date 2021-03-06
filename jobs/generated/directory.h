/******************************************************************************
 * THIS FILE IS GENERATED - ANY EDITS WILL BE OVERWRITTEN
 */

#pragma once

#include "../basejob.h"

#include <QtCore/QVector>


namespace QMatrixClient
{
    // Operations

    class SetRoomAliasJob : public BaseJob
    {
        public:
            explicit SetRoomAliasJob(const QString& roomAlias, const QString& roomId = {});
    };

    class GetRoomIdByAliasJob : public BaseJob
    {
        public:
            /** Construct a URL out of baseUrl and usual parameters passed to
             * GetRoomIdByAliasJob. This function can be used when
             * a URL for GetRoomIdByAliasJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomAlias);

            explicit GetRoomIdByAliasJob(const QString& roomAlias);
            ~GetRoomIdByAliasJob() override;

            const QString& roomId() const;
            const QVector<QString>& servers() const;

        protected:
            Status parseJson(const QJsonDocument& data) override;

        private:
            class Private;
            QScopedPointer<Private> d;
    };

    class DeleteRoomAliasJob : public BaseJob
    {
        public:
            /** Construct a URL out of baseUrl and usual parameters passed to
             * DeleteRoomAliasJob. This function can be used when
             * a URL for DeleteRoomAliasJob is necessary but the job
             * itself isn't.
             */
            static QUrl makeRequestUrl(QUrl baseUrl, const QString& roomAlias);

            explicit DeleteRoomAliasJob(const QString& roomAlias);
    };
} // namespace QMatrixClient
