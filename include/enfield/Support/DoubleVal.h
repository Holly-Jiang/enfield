#ifndef __EFD_DOUBLE_VAL_H__
#define __EFD_DOUBLE_VAL_H__

#include <string>

namespace efd {

    /// \brief Wrapper for double values
    struct DoubleVal {
        /// \brief The double representation.
        double mV;
        /// \brief The string representation of the double.
        std::string mStr;

        DoubleVal();
        /// \brief Parses the string to a double value.
        DoubleVal(std::string str);
    };
};

namespace std {

    /// \brief Overloading std::to_string to work with efd::DoubleVal.
    string to_string(const efd::DoubleVal& val) {
        return val.mStr;
    }

};

#endif
