#include <sys/mman.h>
#include <sys/stat.h>
#include "middle-rewritten.hpp"
#include "options.hpp"
#include "format.hpp"

// the osmium builder will throw asserts if they're enabled? wtf?
#undef NDEBUG
#include <assert.h>

static osmium::Location morton_to_location(uint64_t m) {
    if ((int64_t)m >= 0) {
        return osmium::Location{};
    }

    uint32_t e = 0, o = 0;
    for (int i=0;i<32;i++) {
        e >>= 1;
        o >>= 1;
        if (m & 1) {
            e |= 0x80000000;
        }
        if (m & 2) {
            o |= 0x80000000;
        }
        m >>= 2;
    }
    int32_t lon = e;
    int32_t lat = o;

    // Propatgate the sign from bit 30 to bit 31
    lat <<= 1;
    lat >>= 1;

    assert(((lat & 0x80000000) == 0) == ((lat & 0x40000000) == 0));

    return osmium::Location{(double)lon * 0.0000001, (double)lat * 0.0000001};
}

relindex_t::relindex_t(std::string filename) {
    struct stat st;

    this->f = fopen(filename.c_str(), "rb");
    assert(this->f);

    auto rc = fstat(fileno(this->f), &st);
    assert(rc >= 0);

    this->data = (s64 *)mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fileno(this->f), 0);
    assert(this->data);
    assert(this->data != MAP_FAILED);

    this->next = 0;
    this->size = st.st_size / 8;
}

relindex_t::~relindex_t() {
    assert(!"dtor"); // i guess this is never invoked
    if (this->data) {
        munmap(this->data, 0);
        this->data = nullptr;
    }
    if (this->f) {
        fclose(this->f);
        this->f = nullptr;
    }
}

void middle_rewritten_t::node_set(osmium::Node const &node) {
    //assert(!"node_set");
}

void middle_rewritten_t::way_set(osmium::Way const &way) {
    //assert(!"way_set");
}

void middle_rewritten_t::relation_set(osmium::Relation const &rel) {
    //assert(!"relation_set");
}

size_t middle_rewritten_t::nodes_get_list(osmium::WayNodeList *nodes) const {
    for (auto &n : *nodes) {
        // This should always be the case, however there are times when it's not
        // because if rewritenodes drops node it can't lookup, then there is
        // strange behavior that I don't understand in multipolygons.
        //assert((int64_t)n.ref() < 0);
        n.set_location(morton_to_location(n.ref()));
    }
    return nodes->size();
}

bool middle_rewritten_t::way_get(osmid_t id, osmium::memory::Buffer &buffer) const {
    assert(!"way_get");
    return false;
}

s64 middle_rewritten_t::next() const {
    assert(this->m_relindex->next < this->m_relindex->size);
    return this->m_relindex->data[this->m_relindex->next++];
}

void middle_rewritten_t::skip(int i) const {
    this->m_relindex->next += i;
    assert(this->m_relindex->next < this->m_relindex->size);
}

size_t middle_rewritten_t::rel_way_members_get(
    osmium::Relation const &rel,
    rolelist_t *roles,
    osmium::memory::Buffer &buffer
) const {
    auto idxRelId = this->next();

    for (;idxRelId < rel.id();) {
        auto wayCount = this->next();
        for (auto wayIndex = 0; wayIndex < wayCount; wayIndex++) {
            this->next(); // way id
            this->skip(this->next());
        }
        idxRelId = this->next();
    }

    assert(idxRelId == rel.id());

    auto wayIndex = 0;
    auto wayCount = this->next();

    for (auto const &relMember : rel.members()) {
        if (relMember.type() != osmium::item_type::way) {
            continue;
        }

        assert(wayIndex < wayCount);

        auto idxWayId = this->next();
        assert(idxWayId == relMember.ref());

        {
            osmium::builder::WayBuilder wayBuilder{buffer};
            wayBuilder.set_id(relMember.ref());
            osmium::builder::WayNodeListBuilder nodeListBuilder{buffer, &wayBuilder};

            auto locationCount = this->next();
            for (auto locationIndex=0;locationIndex<locationCount;locationIndex++) {
                auto mortonLocation = this->next();
                if (mortonLocation == 0) {
                    fmt::print("skipping 0 in r={} w={}\n", rel.id(), relMember.ref());
                    continue;
                }
                nodeListBuilder.add_node_ref(mortonLocation, morton_to_location(mortonLocation));
            }
        }
        buffer.commit();
        if (roles) {
            roles->emplace_back(relMember.role());
        }
        wayIndex++;
    }
    assert(wayIndex == wayCount);
    return wayCount;
}

bool middle_rewritten_t::relation_get(
    osmid_t id,
    osmium::memory::Buffer &buffer
) const {
    assert(!"relation_get");
    return false;
}

void middle_rewritten_t::stop(thread_pool_t &) {
    //assert(!"stop");
}

middle_rewritten_t::middle_rewritten_t(options_t const *options) {
    m_relindex = new relindex_t(options->relation_index);
}

idlist_t middle_rewritten_t::relations_using_way(osmid_t) const {
    assert(!"relations_using_way");
    return {};
}

std::shared_ptr<middle_query_t> middle_rewritten_t::get_query_instance() {
    return shared_from_this();
}
