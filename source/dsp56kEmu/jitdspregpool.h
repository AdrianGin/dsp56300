#pragma once

#include <list>
#include <vector>

#include "jitregtypes.h"

namespace dsp56k
{
	class JitBlock;

	class JitDspRegPool
	{
	public:
		enum DspReg
		{
			DspR0,	DspR1,	DspR2,	DspR3,	DspR4,	DspR5,	DspR6,	DspR7,
			DspN0,	DspN1,	DspN2,	DspN3,	DspN4,	DspN5,	DspN6,	DspN7,
			DspM0,	DspM1,	DspM2,	DspM3,	DspM4,	DspM5,	DspM6,	DspM7,

			DspA,	DspB,
			DspX,	DspY,

			DspExtMem,
			DspSR,
			DspLC,
			DspLA,

			TempA, TempB, TempC, TempD, TempE, TempF, TempG, TempH, LastTemp = TempH,

			DspCount
		};

		JitDspRegPool(JitBlock& _block);
		~JitDspRegPool();

		JitReg get(DspReg _reg, bool _read, bool _write);

		void read(const JitReg& _dst, DspReg _src);
		void write(DspReg _dst, const JitReg& _src);

		void lock(DspReg _reg);
		void unlock(DspReg _reg);

		void releaseAll();
		void releaseWritten();

		bool hasWrittenRegs() const { return m_writtenDspRegs != 0; }

		void setRepMode(bool _repMode) { m_repMode = _repMode; }
		bool isInUse(const JitReg128& _xmm) const;
		bool isInUse(const JitReg& _gp) const;

		DspReg aquireTemp();
		void releaseTemp(DspReg _reg);

	private:
		void makeSpace(DspReg _wantedReg);
		void clear();

		void load(JitReg& _dst, DspReg _src);
		void store(DspReg _dst, JitReg& _src);
		void store(DspReg _dst, JitReg128& _src);

		bool release(DspReg _dst);

		template<typename T> void push(std::list<T>& _dst, const T& _src)
		{
			if(m_repMode)
				_dst.push_front(_src);
			else
				_dst.push_back(_src);
		}

		bool isWritten(DspReg _reg) const		{ return m_writtenDspRegs & (1ull<<static_cast<uint64_t>(_reg)); }
		void setWritten(DspReg _reg)			{ m_writtenDspRegs |= (1ull<<static_cast<uint64_t>(_reg)); }											  
		void clearWritten(DspReg _reg)			{ m_writtenDspRegs &= ~(1ull<<static_cast<uint64_t>(_reg)); }

		bool isLocked(DspReg _reg) const		{ return m_lockedGps & (1ull<<static_cast<uint64_t>(_reg)); }
		void setLocked(DspReg _reg)				{ m_lockedGps |= (1ull<<static_cast<uint64_t>(_reg)); }											  
		void clearLocked(DspReg _reg)			{ m_lockedGps &= ~(1ull<<static_cast<uint64_t>(_reg)); }

		template<typename T> class RegisterList
		{
		public:
			RegisterList()
			{
				m_used.reserve(DspCount);
				m_available.reserve(DspCount);
			}

			bool isFull() const					{ return m_available.empty(); }
			bool isUsed(DspReg _reg) const		{ return m_usedMap[_reg].isValid(); }
			bool isUsed(const T& _reg) const
			{
				for(size_t i=0; i<DspCount; ++i)
				{
					if(m_usedMap[i].equals(_reg))
						return true;
				}
				return false;
			}
			bool empty() const					{ return m_used.empty(); }
			size_t available() const			{ return m_available.size(); }
			size_t size() const { return m_used.size(); }

			bool acquire(T& _dst, const DspReg _reg, bool _pushFront)
			{
				const auto existing = m_usedMap[_reg];

				if(existing.isValid())
				{
					if(!_pushFront)
					{
						remove(m_used, _reg);
						m_used.push_back(_reg);
					}
					_dst = existing;
					return true;
				}

				if(isFull())
					return false;

				auto res = m_available.front();
				m_available.erase(m_available.begin());
				m_usedMap[_reg] = res;

				push(m_used, _reg, _pushFront);
				_dst = res;
				return true;
			}

			bool get(T& _dst, const DspReg _reg)
			{
				_dst = m_used[_reg];
				return _dst.isValid();
			}

			bool release(T& _dst, DspReg _reg, bool _pushFront)
			{
				_dst = m_usedMap[_reg];
				if(!_dst.isValid())
					return false;
				push(m_available, _dst, _pushFront);
				remove(m_used, _reg);
				m_usedMap[_reg].reset();
				return true;
			}

			void addHostReg(const T& _hr)
			{
				m_available.push_back(_hr);
			}

			void clear()
			{
				m_available.clear();
				m_used.clear();
				for(size_t i=0; i<DspCount; ++i)
					m_usedMap[i].reset();
			}

			const std::vector<DspReg>& used() const { return m_used; }

		private:
			template<typename TT> static void push(std::vector<TT>& _dst, const TT& _src, bool _pushFront)
			{
				if(_pushFront)
					_dst.insert(_dst.begin(), _src);
				else
					_dst.push_back(_src);
			}
			template<typename TT> static void remove(std::vector<TT>& _dst, const DspReg _src)
			{
				for(auto it = _dst.begin(); it != _dst.end(); ++it)
				{
					if(*it == _src)
					{
						_dst.erase(it);
						break;
					}
				}
			}
			std::vector<T> m_available;
			std::vector<DspReg> m_used;
			T m_usedMap[DspCount];
		};

		JitBlock& m_block;

		uint64_t m_lockedGps;
		uint64_t m_writtenDspRegs;

		RegisterList<JitReg> m_gpList;
		RegisterList<JitReg128> m_xmList;

		std::list<DspReg> m_availableTemps;

		bool m_repMode = false;
	};
}
