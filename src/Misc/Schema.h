#ifndef SCHEMA_H
#define SCHEMA_H

#include <iosfwd>

namespace rtosc {
    struct Ports;
}

namespace zyn {

void dump_json(std::ostream &o,
               const rtosc::Ports &pMaster,
               const rtosc::Ports &pMw);

}

#endif // SCHEMA_H
