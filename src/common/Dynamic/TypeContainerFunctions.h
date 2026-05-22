/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TYPECONTAINER_FUNCTIONS_H
#define TYPECONTAINER_FUNCTIONS_H

/*
 * Here you'll find a list of helper functions to make
 * the TypeContainer usefull.  Without it, its hard
 * to access or mutate the container.
 */

#include "Dynamic/TypeList.h"
#include "Log.h"

#include <sstream>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace Acore
{
    namespace Detail
    {
        template<class T, class = void>
        struct HasToString : std::false_type { };

        template<class T>
        struct HasToString<T, std::void_t<decltype(std::declval<T const&>().ToString())>> : std::true_type { };

        template<class T, class = void>
        struct IsStreamable : std::false_type { };

        template<class T>
        struct IsStreamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T const&>())>>
            : std::true_type { };

        template<class T, class = void>
        struct HasGetGUID : std::false_type { };

        template<class T>
        struct HasGetGUID<T, std::void_t<decltype(std::declval<T const*>()->GetGUID())>> : std::true_type { };

        template<class T, class = void>
        struct HasGetEntry : std::false_type { };

        template<class T>
        struct HasGetEntry<T, std::void_t<decltype(std::declval<T const*>()->GetEntry())>> : std::true_type { };

        template<class T, class = void>
        struct HasGetMapId : std::false_type { };

        template<class T>
        struct HasGetMapId<T, std::void_t<decltype(std::declval<T const*>()->GetMapId())>> : std::true_type { };

        template<class T, class = void>
        struct HasIsInWorld : std::false_type { };

        template<class T>
        struct HasIsInWorld<T, std::void_t<decltype(std::declval<T const*>()->IsInWorld())>> : std::true_type { };

        template<class T, class = void>
        struct HasPosition : std::false_type { };

        template<class T>
        struct HasPosition<T, std::void_t<decltype(std::declval<T const*>()->GetPositionX()),
                                          decltype(std::declval<T const*>()->GetPositionY()),
                                          decltype(std::declval<T const*>()->GetPositionZ())>> : std::true_type { };

        template<class T>
        std::string FormatTypeContainerValue(T const& value)
        {
            if constexpr (HasToString<T>::value)
            {
                return value.ToString();
            }
            else if constexpr (IsStreamable<T>::value)
            {
                std::ostringstream out;
                out << value;
                return out.str();
            }
            else
            {
                return "<unprintable>";
            }
        }

        template<class T>
        std::string FormatTypeContainerObject(T const* obj)
        {
            std::ostringstream out;
            out << "ptr=" << static_cast<void const*>(obj);

            if (!obj)
                return out.str();

            if constexpr (HasGetGUID<T>::value)
                out << " guid=" << FormatTypeContainerValue(obj->GetGUID());

            if constexpr (HasGetEntry<T>::value)
                out << " entry=" << obj->GetEntry();

            if constexpr (HasGetMapId<T>::value)
                out << " map=" << obj->GetMapId();

            if constexpr (HasIsInWorld<T>::value)
                out << " inWorld=" << obj->IsInWorld();

            if constexpr (HasPosition<T>::value)
                out << " pos=(" << obj->GetPositionX() << ", " << obj->GetPositionY() << ", "
                    << obj->GetPositionZ() << ")";

            return out.str();
        }
    }

    // Helpers
    // Insert helpers
    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Insert(ContainerUnorderedMap<SPECIFIC_TYPE, KEY_TYPE>& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* obj)
    {
        auto i = elements._element.find(handle);
        if (i == elements._element.end())
        {
            elements._element[handle] = obj;
            return true;
        }
        else
        {
            if (i->second != obj)
            {
                LOG_ERROR("entities.object",
                          "TypeMapContainer duplicate key collision before ASSERT: objectType={} keyType={} key={} "
                          "existing=[{}] incoming=[{}] containerSize={}",
                          typeid(SPECIFIC_TYPE).name(), typeid(KEY_TYPE).name(),
                          Detail::FormatTypeContainerValue(handle),
                          Detail::FormatTypeContainerObject(i->second),
                          Detail::FormatTypeContainerObject(obj), elements._element.size());
            }

            ASSERT(i->second == obj, "Object with certain key already in but objects are different!");
            return false;
        }
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Insert(ContainerUnorderedMap<TypeNull, KEY_TYPE>& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class T>
    bool Insert(ContainerUnorderedMap<T, KEY_TYPE>& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class H, class T>
    bool Insert(ContainerUnorderedMap<TypeList<H, T>, KEY_TYPE>& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* obj)
    {
        bool ret = Insert(elements._elements, handle, obj);
        return ret ? ret : Insert(elements._TailElements, handle, obj);
    }

    // Find helpers
    template<class SPECIFIC_TYPE, class KEY_TYPE>
    SPECIFIC_TYPE* Find(ContainerUnorderedMap<SPECIFIC_TYPE, KEY_TYPE> const& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* /*obj*/)
    {
        auto i = elements._element.find(handle);
        if (i == elements._element.end())
        {
            return nullptr;
        }
        else
        {
            return i->second;
        }
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE>
    SPECIFIC_TYPE* Find(ContainerUnorderedMap<TypeNull, KEY_TYPE> const& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class T>
    SPECIFIC_TYPE* Find(ContainerUnorderedMap<T, KEY_TYPE> const& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class H, class T>
    SPECIFIC_TYPE* Find(ContainerUnorderedMap<TypeList<H, T>, KEY_TYPE> const& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* /*obj*/)
    {
        SPECIFIC_TYPE* ret = Find(elements._elements, handle, (SPECIFIC_TYPE*)nullptr);
        return ret ? ret : Find(elements._TailElements, handle, (SPECIFIC_TYPE*)nullptr);
    }

    // Erase helpers
    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Remove(ContainerUnorderedMap<SPECIFIC_TYPE, KEY_TYPE>& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* /*obj*/)
    {
        elements._element.erase(handle);
        return true;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Remove(ContainerUnorderedMap<TypeNull, KEY_TYPE>& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class T>
    bool Remove(ContainerUnorderedMap<T, KEY_TYPE>& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class H, class T>
    bool Remove(ContainerUnorderedMap<TypeList<H, T>, KEY_TYPE>& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* /*obj*/)
    {
        bool ret = Remove(elements._elements, handle, (SPECIFIC_TYPE*)nullptr);
        return ret ? ret : Remove(elements._TailElements, handle, (SPECIFIC_TYPE*)nullptr);
    }

    // Count helpers
    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Size(ContainerUnorderedMap<SPECIFIC_TYPE, KEY_TYPE> const& elements, std::size_t* size, SPECIFIC_TYPE* /*obj*/)
    {
        *size = elements._element.size();
        return true;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Size(ContainerUnorderedMap<TypeNull, KEY_TYPE> const& /*elements*/, std::size_t* /*size*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class T>
    bool Size(ContainerUnorderedMap<T, KEY_TYPE> const& /*elements*/, std::size_t* /*size*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class H, class T>
    bool Size(ContainerUnorderedMap<TypeList<H, T>, KEY_TYPE> const& elements, std::size_t* size, SPECIFIC_TYPE* /*obj*/)
    {
        bool ret = Size(elements._elements, size, (SPECIFIC_TYPE*)nullptr);
        return ret ? ret : Size(elements._TailElements, size, (SPECIFIC_TYPE*)nullptr);
    }

    /* ContainerMapList Helpers */
    // count functions
    template<class SPECIFIC_TYPE>
    std::size_t Count(const ContainerMapList<SPECIFIC_TYPE>& elements, SPECIFIC_TYPE* /*fake*/)
    {
        return elements._element.getSize();
    }

    template<class SPECIFIC_TYPE>
    std::size_t Count(const ContainerMapList<TypeNull>& /*elements*/, SPECIFIC_TYPE* /*fake*/)
    {
        return 0;
    }

    template<class SPECIFIC_TYPE, class T>
    std::size_t Count(const ContainerMapList<T>& /*elements*/, SPECIFIC_TYPE* /*fake*/)
    {
        return 0;
    }

    template<class SPECIFIC_TYPE, class T>
    std::size_t Count(const ContainerMapList<TypeList<SPECIFIC_TYPE, T>>& elements, SPECIFIC_TYPE* fake)
    {
        return Count(elements._elements, fake);
    }

    template<class SPECIFIC_TYPE, class H, class T>
    std::size_t Count(const ContainerMapList<TypeList<H, T>>& elements, SPECIFIC_TYPE* fake)
    {
        return Count(elements._TailElements, fake);
    }

    // non-const insert functions
    template<class SPECIFIC_TYPE>
    SPECIFIC_TYPE* Insert(ContainerMapList<SPECIFIC_TYPE>& elements, SPECIFIC_TYPE* obj)
    {
        //elements._element[hdl] = obj;
        obj->AddToGrid(elements._element);
        return obj;
    }

    template<class SPECIFIC_TYPE>
    SPECIFIC_TYPE* Insert(ContainerMapList<TypeNull>& /*elements*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr;
    }

    // this is a missed
    template<class SPECIFIC_TYPE, class T>
    SPECIFIC_TYPE* Insert(ContainerMapList<T>& /*elements*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr;                                        // a missed
    }

    // Recursion
    template<class SPECIFIC_TYPE, class H, class T>
    SPECIFIC_TYPE* Insert(ContainerMapList<TypeList<H, T>>& elements, SPECIFIC_TYPE* obj)
    {
        SPECIFIC_TYPE* t = Insert(elements._elements, obj);
        return (t != nullptr ? t : Insert(elements._TailElements, obj));
    }

    //// non-const remove method
    //template<class SPECIFIC_TYPE> SPECIFIC_TYPE* Remove(ContainerMapList<SPECIFIC_TYPE> & /*elements*/, SPECIFIC_TYPE *obj)
    //{
    //    obj->GetGridRef().unlink();
    //    return obj;
    //}

    //template<class SPECIFIC_TYPE> SPECIFIC_TYPE* Remove(ContainerMapList<TypeNull> &/*elements*/, SPECIFIC_TYPE * /*obj*/)
    //{
    //    return nullptr;
    //}

    //// this is a missed
    //template<class SPECIFIC_TYPE, class T> SPECIFIC_TYPE* Remove(ContainerMapList<T> &/*elements*/, SPECIFIC_TYPE * /*obj*/)
    //{
    //    return nullptr;                                        // a missed
    //}

    //template<class SPECIFIC_TYPE, class T, class H> SPECIFIC_TYPE* Remove(ContainerMapList<TypeList<H, T> > &elements, SPECIFIC_TYPE *obj)
    //{
    //    // The head element is bad
    //    SPECIFIC_TYPE* t = Remove(elements._elements, obj);
    //    return ( t != nullptr ? t : Remove(elements._TailElements, obj));
    //}

    /* ContainerVector Helpers */
    // count functions
    template<class SPECIFIC_TYPE>
    std::size_t Count(const ContainerVector<SPECIFIC_TYPE>& elements, SPECIFIC_TYPE* /*fake*/)
    {
        return elements._element.getSize();
    }

    template<class SPECIFIC_TYPE>
    std::size_t Count(const ContainerVector<TypeNull>& /*elements*/, SPECIFIC_TYPE* /*fake*/)
    {
        return 0;
    }

    template<class SPECIFIC_TYPE, class T>
    std::size_t Count(const ContainerVector<T>& /*elements*/, SPECIFIC_TYPE* /*fake*/)
    {
        return 0;
    }

    template<class SPECIFIC_TYPE, class T>
    std::size_t Count(const ContainerVector<TypeList<SPECIFIC_TYPE, T>>& elements, SPECIFIC_TYPE* fake)
    {
        return Count(elements._elements, fake);
    }

    template<class SPECIFIC_TYPE, class H, class T>
    std::size_t Count(const ContainerVector<TypeList<H, T>>& elements, SPECIFIC_TYPE* fake)
    {
        return Count(elements._TailElements, fake);
    }

    // non-const insert functions
    template<class SPECIFIC_TYPE>
    SPECIFIC_TYPE* Insert(ContainerVector<SPECIFIC_TYPE>& elements, SPECIFIC_TYPE* obj)
    {
        elements._element.push_back(obj);
        return obj;
    }

    template<class SPECIFIC_TYPE>
    SPECIFIC_TYPE* Insert(ContainerVector<TypeNull>& /*elements*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr;
    }

    // this is a missed
    template<class SPECIFIC_TYPE, class T>
    SPECIFIC_TYPE* Insert(ContainerVector<T>& /*elements*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr;                                        // a missed
    }

    // Recursion
    template<class SPECIFIC_TYPE, class H, class T>
    SPECIFIC_TYPE* Insert(ContainerVector<TypeList<H, T>>& elements, SPECIFIC_TYPE* obj)
    {
        SPECIFIC_TYPE* t = Insert(elements._elements, obj);
        return (t != nullptr ? t : Insert(elements._TailElements, obj));
    }

    // non-const remove method
    template<class SPECIFIC_TYPE> SPECIFIC_TYPE* Remove(ContainerVector<SPECIFIC_TYPE>& elements, SPECIFIC_TYPE *obj)
    {
        // Simple vector find/swap/pop, this container should be very lightly used
        // so I don't suspect the linear search complexity to be an issue
        auto itr = std::find(elements._element.begin(), elements._element.end(), obj);
        if (itr != elements._element.end())
        {
            // Swap the element to be removed with the last element
            std::swap(*itr, elements._element.back());

            // Remove the last element (which is now the element we wanted to remove)
            elements._element.pop_back();
        }
        return obj;
    }

    template<class SPECIFIC_TYPE> SPECIFIC_TYPE* Remove(ContainerVector<TypeNull> &/*elements*/, SPECIFIC_TYPE * /*obj*/)
    {
        return nullptr;
    }

    // this is a missed
    template<class SPECIFIC_TYPE, class T> SPECIFIC_TYPE* Remove(ContainerVector<T> &/*elements*/, SPECIFIC_TYPE * /*obj*/)
    {
        return nullptr;                                        // a missed
    }

    template<class SPECIFIC_TYPE, class T, class H> SPECIFIC_TYPE* Remove(ContainerVector<TypeList<H, T> > &elements, SPECIFIC_TYPE *obj)
    {
        // The head element is bad
        SPECIFIC_TYPE* t = Remove(elements._elements, obj);
        return ( t != nullptr ? t : Remove(elements._TailElements, obj));
    }
}
#endif
