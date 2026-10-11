#pragma once

#include "./Backend.hpp"
#include "../allocator/Swapchain.hpp"
#include "../output/Output.hpp"
#include <abyssutils/memory/WeakPtr.hpp>

namespace Abyssal {
    class CBackend;
    class IAllocator;

    class CNullBackend : public IBackendImplementation {
      public:
        virtual ~CNullBackend();
        virtual eBackendType                                               type();
        virtual bool                                                       start();
        virtual std::vector<Abyssutils::Memory::CSharedPointer<SPollFD>>    pollFDs();
        virtual int                                                        drmFD();
        virtual bool                                                       dispatchEvents();
        virtual uint32_t                                                   capabilities();
        virtual bool                                                       setCursor(Abyssutils::Memory::CSharedPointer<IBuffer> buffer, const Abyssutils::Math::Vector2D& hotspot);
        virtual void                                                       onReady();
        virtual std::vector<SDRMFormat>                                    getRenderFormats();
        virtual std::vector<SDRMFormat>                                    getCursorFormats();
        virtual bool                                                       createOutput(const std::string& name = "");
        virtual Abyssutils::Memory::CSharedPointer<IAllocator>              preferredAllocator();
        virtual std::vector<Abyssutils::Memory::CSharedPointer<IAllocator>> getAllocators();
        virtual Abyssutils::Memory::CWeakPointer<IBackendImplementation>    getPrimary();

        Abyssutils::Memory::CWeakPointer<CNullBackend>                      self;
        virtual int                                                        drmRenderNodeFD();

        void                                                               setFormats(const std::vector<SDRMFormat>& fmts);

      private:
        CNullBackend(Abyssutils::Memory::CSharedPointer<CBackend> backend_);

        Abyssutils::Memory::CWeakPointer<CBackend> backend;

        std::vector<SDRMFormat>                   m_formats;

        friend class CBackend;
        friend class CHeadlessOutput;
    };
};
