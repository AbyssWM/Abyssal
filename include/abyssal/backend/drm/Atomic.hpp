#pragma once

#include "../DRM.hpp"

namespace Abyssal {
    class CDRMAtomicRequest;

    class CDRMAtomicImpl : public IDRMImplementation {
      public:
        CDRMAtomicImpl(Abyssutils::Memory::CSharedPointer<CDRMBackend> backend_);
        virtual bool                                         commit(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, SDRMConnectorCommitData& data);
        virtual bool                                         reset();
        virtual bool                                         moveCursor(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, bool skipSchedule = false);

        Abyssutils::Memory::CUniquePointer<CDRMAtomicRequest> prepareAsync(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, SDRMConnectorCommitData& data,
                                                                          uint32_t& flags);
        void finalizeAsync(CDRMAtomicRequest& request, Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, SDRMConnectorCommitData& data, bool success);

      private:
        bool                                         prepareConnector(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, SDRMConnectorCommitData& data);

        Abyssutils::Memory::CWeakPointer<CDRMBackend> backend;

        friend class CDRMAtomicRequest;
    };

    class CDRMAtomicRequest {
      public:
        struct SAsyncResult {
            bool submitted = false;
            int  error     = 0;
        };

        CDRMAtomicRequest(Abyssutils::Memory::CWeakPointer<CDRMBackend> backend);
        ~CDRMAtomicRequest();

        void         setConnector(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector);
        void         addConnector(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, SDRMConnectorCommitData& data);
        bool         restateConnectors(Abyssutils::Memory::CSharedPointer<SDRMConnector> self);
        void         addConnectorModeset(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, SDRMConnectorCommitData& data);
        void         addConnectorCursor(Abyssutils::Memory::CSharedPointer<SDRMConnector> connector, SDRMConnectorCommitData& data);
        bool         commit(uint32_t flagssss);
        void         add(uint32_t id, uint32_t prop, uint64_t val);
        bool         addRaw(uint32_t id, uint32_t prop, uint64_t val) noexcept;
        SAsyncResult submitAsync(int drmFD, uint32_t flags, uint64_t commitID) noexcept;
        void         planeProps(Abyssutils::Memory::CSharedPointer<SDRMPlane> plane, Abyssutils::Memory::CSharedPointer<CDRMFB> fb, uint32_t crtc, Abyssutils::Math::Vector2D pos,
                                eOutputColorRange colorRange = ABYSSAL_OUTPUT_COLOR_RANGE_AUTO);
        void         planePropsPos(Abyssutils::Memory::CSharedPointer<SDRMPlane> plane, Abyssutils::Math::Vector2D pos);

        void         rollback(SDRMConnectorCommitData& data);
        void         apply(SDRMConnectorCommitData& data);

        bool         failed = false;

      private:
        void                                             destroyBlob(uint32_t id);
        void                                             commitBlob(uint32_t* current, uint32_t next);
        void                                             rollbackBlob(uint32_t* current, uint32_t next);

        Abyssutils::Memory::CWeakPointer<CDRMBackend>     backend;
        drmModeAtomicReq*                                req = nullptr;
        Abyssutils::Memory::CSharedPointer<SDRMConnector> conn;

        // mode blobs minted by restateConnectors, owned by this request
        std::vector<uint32_t> borrowedModeBlobs;
    };
};
