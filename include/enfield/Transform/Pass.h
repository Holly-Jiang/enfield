#ifndef __EFD_PASS_H__
#define __EFD_PASS_H__

#include <memory>

namespace efd {
    class QModule;

    /// \brief Base class for implementation of QModule passes.
    /// This information will be used when the QModule's function
    /// is called.
    class Pass {
        public:
            typedef Pass* Ref;
            typedef std::shared_ptr<Pass> sRef;

            enum Kind {
                K_VOID,
                K_GEN
            };

        private:
            Kind mK;

        protected:
            Pass(Kind k);

        public:
            /// \brief Runs the pass in the given QModule and returns true if it has
            /// modified \p qmod.
            virtual bool run(QModule* qmod) = 0;

            /// \brief Gets the kind of this pass.
            Kind getKind() const;
    };

    /// \brief Should serve as base class for classes that produces
    /// some data of type \em T.
    template <typename T>
        class PassT : public Pass {
            protected:
                T mData;

            public:
                PassT();

                /// \brief Gets the resulting data.
                ///
                /// This should return the data generated by the processing of the
                /// \em run function of the \em Pass class.
                T getData() const;
                T& getData();

                static bool ClassOf(Pass* ref);
        };

    // There is no such function when it is a \em void pass.
    template <>
        class PassT<void> : public Pass {
            public:
                PassT();
                static bool ClassOf(Pass* ref);
        };
};

template <typename T>
efd::PassT<T>::PassT() : Pass(K_GEN) {
}

template <typename T>
T efd::PassT<T>::getData() const {
    return mData;
}

template <typename T>
T& efd::PassT<T>::getData() {
    return mData;
}

template <typename T>
bool efd::PassT<T>::ClassOf(Pass* ref) {
    return ref->getKind() == K_GEN;
}

#endif
