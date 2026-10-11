#pragma once

#include "../DRM.hpp"

namespace Abyssal {
    class CDRMLegacyImpl : public IDRMImplementation {
      public:
        CDRMLegacyImpl(Abyssutils::Memory::CSharedPointer<CDRMBackend> backend_);
        virtual bool commit(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, SDRMConnectorCommitData& data);
        virtual bool reset();
        virtual bool moveCursor(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, bool skipSchedule = false);

      private:
        bool                                         commitInternal(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, SDRMConnectorCommitData& data);
        bool                                         testInternal(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, SDRMConnectorCommitData& data);

        Abyssutils::Memory::CWeakPointer<CDRMBackend> backend;
    };
};
