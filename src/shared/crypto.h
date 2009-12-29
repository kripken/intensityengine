/* Elliptic curve cryptography based on NIST DSS prime curves. */

static int parsedigits(ushort *digits, int maxlen, const char *s)
{
    int slen = 0;
    while(isxdigit(s[slen])) slen++;
    int len = (slen+2*sizeof(ushort)-1)/(2*sizeof(ushort));
    if(len>maxlen) return 0;
    memset(digits, 0, len*sizeof(ushort));
    loopi(slen)
    {
        int c = s[slen-i-1];
        if(isalpha(c)) c = toupper(c) - 'A' + 10;
        else if(isdigit(c)) c -= '0';
        else return 0;
        digits[i/(2*sizeof(ushort))] |= c<<(4*(i%(2*sizeof(ushort)))); 
    }
    return len;
}

#define BI_DIGIT_BITS 16
#define BI_DIGIT_MASK ((1<<BI_DIGIT_BITS)-1)

template<int BI_DIGITS> struct bigint
{
    typedef ushort digit;
    typedef uint dbldigit;

    int len;
    digit digits[BI_DIGITS];

    bigint() {}
    bigint(digit n) { if(n) { len = 1; digits[0] = n; } else len = 0; }
    bigint(const char *s) { parse(s); }
    template<int Y_DIGITS> bigint(const bigint<Y_DIGITS> &y) { *this = y; }

    void parse(const char *s)
    {
        len = parsedigits(digits, BI_DIGITS, s);
        shrink();
    }

    void zero() { len = 0; }

    void print(FILE *out) const 
    { 
        vector<char> buf;
        printdigits(buf);
        buf.add('\0');
        fputs(buf.getbuf(), out);
    }

    void printdigits(vector<char> &buf) const
    {
        loopi(len)
        {
            digit d = digits[len-i-1];
            loopj(BI_DIGIT_BITS/4)
            {
                uint shift = BI_DIGIT_BITS - (j+1)*4;
                int val = (d >> shift) & 0xF;
                if(val < 10) buf.add('0' + val);
                else buf.add('a' + val - 10);
            }
        }
    }

    template<int Y_DIGITS> bigint &operator=(const bigint<Y_DIGITS> &y)
    {
        len = y.len;
        memcpy(digits, y.digits, len*sizeof(digit));
        return *this;
    }

    bool iszero() const { return !len; }
    bool isone() const { return len==1 && digits[0]==1; }

    int numbits() const
    {
        if(!len) return 0;
        int bits = len*BI_DIGIT_BITS;
        digit last = digits[len-1], mask = 1<<(BI_DIGIT_BITS-1);
        while(mask)
        {
            if(last&mask) return bits;
            bits--;
            mask >>= 1;
        }
        return 0;
    }

    bool hasbit(int n) const { return n/BI_DIGIT_BITS < len && ((digits[n/BI_DIGIT_BITS]>>(n%BI_DIGIT_BITS))&1); }

    template<int X_DIGITS, int Y_DIGITS> bigint &add(const bigint<X_DIGITS> &x, const bigint<Y_DIGITS> &y)
    {
        dbldigit carry = 0;
        int maxlen = max(x.len, y.len), i;
        for(i = 0; i < y.len || carry; i++)
        {
             carry += (i < x.len ? (dbldigit)x.digits[i] : 0) + (i < y.len ? (dbldigit)y.digits[i] : 0);
             digits[i] = (digit)carry;
             carry >>= BI_DIGIT_BITS;
        }
        if(i < x.len && this != &x) memcpy(&digits[i], &x.digits[i], (x.len - i)*sizeof(digit));
        len = max(i, maxlen);
        return *this;
    }
    template<int Y_DIGITS> bigint &add(const bigint<Y_DIGITS> &y) { return add(*this, y); }

    template<int X_DIGITS, int Y_DIGITS> bigint &sub(const bigint<X_DIGITS> &x, const bigint<Y_DIGITS> &y)
    {
        ASSERT(x >= y);
        dbldigit borrow = 0;
        int i;
        for(i = 0; i < y.len || borrow; i++)
        {
             borrow = (1<<BI_DIGIT_BITS) + (dbldigit)x.digits[i] - (i<y.len ? (dbldigit)y.digits[i] : 0) - borrow;
             digits[i] = (digit)borrow;
             borrow = (borrow>>BI_DIGIT_BITS)^1;
        }
        if(i < x.len && this != &x) memcpy(&digits[i], &x.digits[i], (x.len - i)*sizeof(digit));
        len = x.len;
        shrink();
        return *this;
    }
    template<int Y_DIGITS> bigint &sub(const bigint<Y_DIGITS> &y) { return sub(*this, y); }

    void shrink() { while(len && !digits[len-1]) len--; }

    template<int X_DIGITS, int Y_DIGITS> bigint &mul(const bigint<X_DIGITS> &x, const bigint<Y_DIGITS> &y)
    {
        if(!x.len || !y.len) { len = 0; return *this; }
        memset(digits, 0, y.len*sizeof(digit));
        loopi(x.len)
        {
            dbldigit carry = 0;
            loopj(y.len)
            {
                carry += (dbldigit)x.digits[i] * (dbldigit)y.digits[j] + (dbldigit)digits[i+j];
                digits[i+j] = (digit)carry;
                carry >>= BI_DIGIT_BITS;
            }
            digits[i+y.len] = carry;
        }
        len = x.len + y.len;
        shrink();
        return *this;
    }

    template<int X_DIGITS> bigint &rshift(const bigint<X_DIGITS> &x, int n)
    {
        if(!len || !n) return *this;
        int dig = (n-1)/BI_DIGIT_BITS;
        n = ((n-1) % BI_DIGIT_BITS)+1;
        digit carry = digit(x.digits[dig]>>n);
        loopi(len-dig-1)
        {
            digit tmp = x.digits[i+dig+1];
            digits[i] = digit((tmp<<(BI_DIGIT_BITS-n)) | carry);
            carry = digit(tmp>>n);
        }
        digits[len-dig-1] = carry;
        len -= dig + (n>>BI_DIGIT_BITS);
        shrink();
        return *this;
    }
    bigint &rshift(int n) { return rshift(*this, n); }

    template<int X_DIGITS> bigint &lshift(const bigint<X_DIGITS> &x, int n)
    {
        if(!len || !n) return *this;
        int dig = n/BI_DIGIT_BITS;
        n %= BI_DIGIT_BITS;
        digit carry = 0;
        for(int i = len-1; i>=0; i--)
        {
            digit tmp = x.digits[i];
            digits[i+dig] = digit((tmp<<n) | carry);
            carry = digit(tmp>>(BI_DIGIT_BITS-n));
        }
        len += dig;
        if(carry) digits[len++] = carry;
        if(dig) memset(digits, 0, dig*sizeof(digit));
        return *this;
    }
    bigint &lshift(int n) { return lshift(*this, n); }

    template<int Y_DIGITS> bool operator==(const bigint<Y_DIGITS> &y) const
    {
        if(len!=y.len) return false;
        for(int i = len-1; i>=0; i--) if(digits[i]!=y.digits[i]) return false;
        return true;
    }
    template<int Y_DIGITS> bool operator!=(const bigint<Y_DIGITS> &y) const { return !(*this==y); }
    template<int Y_DIGITS> bool operator<(const bigint<Y_DIGITS> &y) const
    {
        if(len<y.len) return true;
        if(len>y.len) return false;
        for(int i = len-1; i>=0; i--)
        {
            if(digits[i]<y.digits[i]) return true;
            if(digits[i]>y.digits[i]) return false;
        }
        return false;
    }
    template<int Y_DIGITS> bool operator>(const bigint<Y_DIGITS> &y) const { return y<*this; }
    template<int Y_DIGITS> bool operator<=(const bigint<Y_DIGITS> &y) const { return !(y<*this); }
    template<int Y_DIGITS> bool operator>=(const bigint<Y_DIGITS> &y) const { return !(*this<y); }
};

#define GF_BITS         192
#define GF_DIGITS       ((GF_BITS+BI_DIGIT_BITS-1)/BI_DIGIT_BITS)

typedef bigint<GF_DIGITS+1> gfint;

/* NIST prime Galois fields.
 * Currently only supports NIST P-192, where P=2^192-2^64-1.
 */
struct gfield : gfint
{
    static const gfield P;

    gfield() {}
    gfield(digit n) : gfint(n) {}
    gfield(const char *s) : gfint(s) {}

    template<int Y_DIGITS> gfield(const bigint<Y_DIGITS> &y) : gfint(y) {}
    
    template<int Y_DIGITS> gfield &operator=(const bigint<Y_DIGITS> &y)
    { 
        gfint::operator=(y);
        return *this;
    }

    template<int X_DIGITS, int Y_DIGITS> gfield &add(const bigint<X_DIGITS> &x, const bigint<Y_DIGITS> &y)
    {
        gfint::add(x, y);
        if(*this >= P) gfint::sub(*this, P);
        return *this;
    }
    template<int Y_DIGITS> gfield &add(const bigint<Y_DIGITS> &y) { return add(*this, y); }

    template<int X_DIGITS> gfield &mul2(const bigint<X_DIGITS> &x) { return add(x, x); } 
    gfield &mul2() { return mul2(*this); }

    template<int X_DIGITS> gfield &div2(const bigint<X_DIGITS> &x) 
    {
        if(hasbit(0)) { gfint::add(x, P); rshift(1); } 
        else rshift(x, 1);
        return *this;
    }
    gfield &div2() { return div2(*this); }

    template<int X_DIGITS, int Y_DIGITS> gfield &sub(const bigint<X_DIGITS> &x, const bigint<Y_DIGITS> &y)
    {
        if(x < y)
        {
            gfint tmp; /* necessary if this==&y, using this instead would clobber y */
            tmp.add(x, P);
            gfint::sub(tmp, y);
        }
        else gfint::sub(x, y);
        return *this;
    }
    template<int Y_DIGITS> gfield &sub(const bigint<Y_DIGITS> &y) { return sub(*this, y); }

    template<int X_DIGITS> gfield &neg(const bigint<X_DIGITS> &x)
    {
        gfint::sub(P, x);
        return *this;
    }
    gfield &neg() { return neg(*this); }

    template<int X_DIGITS> gfield &square(const bigint<X_DIGITS> &x) { return mul(x, x); }
    gfield &square() { return square(*this); }

    template<int X_DIGITS, int Y_DIGITS> gfield &mul(const bigint<X_DIGITS> &x, const bigint<Y_DIGITS> &y)
    {
        bigint<X_DIGITS+Y_DIGITS> result;
        result.mul(x, y);
        reduce(result);
        return *this;
    }
    template<int Y_DIGITS> gfield &mul(const bigint<Y_DIGITS> &y) { return mul(*this, y); }

    template<int RESULT_DIGITS> void reduce(const bigint<RESULT_DIGITS> &result)
    {
#if GF_BITS==192
        len = min(result.len, GF_DIGITS);
        memcpy(digits, result.digits, len*sizeof(digit));
        shrink();

        if(result.len > 192/BI_DIGIT_BITS)
        {
            gfield s;
            memcpy(s.digits, &result.digits[192/BI_DIGIT_BITS], min(result.len-192/BI_DIGIT_BITS, 64/BI_DIGIT_BITS)*sizeof(digit));
            if(result.len < 256/BI_DIGIT_BITS) memset(&s.digits[result.len-192/BI_DIGIT_BITS], 0, (256/BI_DIGIT_BITS-result.len)*sizeof(digit));
            memcpy(&s.digits[64/BI_DIGIT_BITS], s.digits, 64/BI_DIGIT_BITS*sizeof(digit));
            s.len = 128/BI_DIGIT_BITS;
            s.shrink();
            add(s);

            if(result.len > 256/BI_DIGIT_BITS)
            {
                memset(s.digits, 0, 64/BI_DIGIT_BITS*sizeof(digit));
                memcpy(&s.digits[64/BI_DIGIT_BITS], &result.digits[256/BI_DIGIT_BITS], min(result.len-256/BI_DIGIT_BITS, 64/BI_DIGIT_BITS)*sizeof(digit));
                if(result.len < 320/BI_DIGIT_BITS) memset(&s.digits[result.len+(64-256)/BI_DIGIT_BITS], 0, (320/BI_DIGIT_BITS-result.len)*sizeof(digit));
                memcpy(&s.digits[128/BI_DIGIT_BITS], &s.digits[64/BI_DIGIT_BITS], 64/BI_DIGIT_BITS*sizeof(digit));
                s.len = GF_DIGITS;
                s.shrink();
                add(s);

                if(result.len > 320/BI_DIGIT_BITS)
                {
                    memcpy(s.digits, &result.digits[320/BI_DIGIT_BITS], min(result.len-320/BI_DIGIT_BITS, 64/BI_DIGIT_BITS)*sizeof(digit));
                    if(result.len < 384/BI_DIGIT_BITS) memset(&s.digits[result.len-320/BI_DIGIT_BITS], 0, (384/BI_DIGIT_BITS-result.len)*sizeof(digit));
                    memcpy(&s.digits[64/BI_DIGIT_BITS], s.digits, 64/BI_DIGIT_BITS*sizeof(digit));
                    memcpy(&s.digits[128/BI_DIGIT_BITS], s.digits, 64/BI_DIGIT_BITS*sizeof(digit));
                    s.len = GF_DIGITS;
                    s.shrink();
                    add(s);
                }
            }
        }
        else if(*this >= P) gfint::sub(*this, P);
#else
#error Unsupported GF
#endif
    }

    template<int X_DIGITS, int Y_DIGITS> gfield &pow(const bigint<X_DIGITS> &x, const bigint<Y_DIGITS> &y)
    {
        gfield a(x);
        if(y.hasbit(0)) *this = a;
        else 
        { 
            len = 1; 
            digits[0] = 1; 
            if(!y.len) return *this;
        }
        for(int i = 1, j = y.numbits(); i < j; i++)
        {
            a.square();
            if(y.hasbit(i)) mul(a);
        }
        return *this;
    }
    template<int Y_DIGITS> gfield &pow(const bigint<Y_DIGITS> &y) { return pow(*this, y); }
    
    bool invert(const gfield &x)
    {
        if(!x.len) return false;
        gfint u(x), v(P), A((gfint::digit)1), C((gfint::digit)0);
        while(!u.iszero())
        {
            int ushift = 0, ashift = 0;
            while(!u.hasbit(ushift))
            {
                ushift++;
                if(A.hasbit(ashift))
                { 
                    if(ashift) { A.rshift(ashift); ashift = 0; } 
                    A.add(P); 
                }
                ashift++;
            }
            if(ushift) u.rshift(ushift);
            if(ashift) A.rshift(ashift);
            int vshift = 0, cshift = 0;
            while(!v.hasbit(vshift))
            {
                vshift++;
                if(C.hasbit(cshift))
                { 
                    if(cshift) { C.rshift(cshift); cshift = 0; } 
                    C.add(P); 
                }
                cshift++;
            }
            if(vshift) v.rshift(vshift);
            if(cshift) C.rshift(cshift);
            if(u >= v)
            {
                u.sub(v);
                if(A < C) A.add(P);
                A.sub(C);
            }
            else
            {
                v.sub(v, u);
                if(C < A) C.add(P);
                C.sub(A);
            }    
        }
        if(C >= P) gfint::sub(C, P);
        else { len = C.len; memcpy(digits, C.digits, len*sizeof(digit)); }
        ASSERT(*this < P);
        return true;
    }    
    void invert() { invert(*this); }

    template<int X_DIGITS> static int legendre(const bigint<X_DIGITS> &x)
    {
        static const gfint Psub1div2(gfint(P).sub(bigint<1>(1)).rshift(1));
        gfield L;
        L.pow(x, Psub1div2);
        if(!L.len) return 0;
        if(L.len==1) return 1;
        return -1;
    }
    int legendre() const { return legendre(*this); }

    bool sqrt(const gfield &x)
    {
        if(!x.len) { len = 0; return true; }
#if GF_BITS==224
#error Unsupported GF
#else
        ASSERT((P.digits[0]%4)==3);
        static const gfint Padd1div4(gfint(P).add(bigint<1>(1)).rshift(2));
        switch(legendre(x))
        {
            case 0: len = 0; return true;
            case -1: return false;
            default: pow(x, Padd1div4); return true;
        } 
#endif
    }
    bool sqrt() { return sqrt(*this); }
};

struct ecjacobian
{
    static const gfield B;
    static const ecjacobian base;
    static const ecjacobian origin;

    gfield x, y, z;

    ecjacobian() {}
    ecjacobian(const gfield &x, const gfield &y) : x(x), y(y), z(bigint<1>(1)) {}
    ecjacobian(const gfield &x, const gfield &y, const gfield &z) : x(x), y(y), z(z) {}

    void mul2()
    {
        if(z.iszero()) return;
        else if(y.iszero()) { *this = origin; return; }
        gfield a, b, c, d;
        d.sub(x, c.square(z));
        d.mul(c.add(x));
        c.mul2(d).add(d);
        z.mul(y).add(z);
        a.square(y);
        b.mul2(a);
        d.mul2(x).mul(b);
        x.square(c).sub(d).sub(d);
        a.square(b).add(a);
        y.sub(d, x).mul(c).sub(a);
    }

    void add(const ecjacobian &q)
    {
        if(q.z.iszero()) return;
        else if(z.iszero()) { *this = q; return; }
        gfield a, b, c, d, e, f;
        a.square(z);
        b.mul(q.y, a).mul(z);
        a.mul(q.x);
        if(q.z.isone())
        {
            c.add(x, a);
            d.add(y, b);
            a.sub(x, a);
            b.sub(y, b);
        }
        else
        {
            f.mul(y, e.square(q.z)).mul(q.z);
            e.mul(x);
            c.add(e, a);
            d.add(f, b);
            a.sub(e, a);
            b.sub(f, b);
        }
        if(a.iszero()) { if(b.iszero()) mul2(); else *this = origin; return; }
        if(!q.z.isone()) z.mul(q.z);
        z.mul(a);
        x.square(b).sub(f.mul(c, e.square(a)));
        y.sub(f, x).sub(x).mul(b).sub(e.mul(a).mul(d)).div2();
    }
 
    template<int Q_DIGITS> void mul(const ecjacobian &p, const bigint<Q_DIGITS> q)
    {
        *this = origin;
        for(int i = q.numbits()-1; i >= 0; i--)
        {
            mul2();
            if(q.hasbit(i)) add(p);
        }
    }
    template<int Q_DIGITS> void mul(const bigint<Q_DIGITS> q) { ecjacobian tmp(*this); mul(tmp, q); }

    void normalize()
    {
        if(z.iszero() || z.isone()) return;
        gfield tmp;
        z.invert();
        tmp.square(z);
        x.mul(tmp);
        y.mul(tmp).mul(z);
        z = bigint<1>(1);
    }

    bool calcy(bool ybit)
    {
        gfield y2, tmp;
        y2.square(x).mul(x).sub(tmp.add(x, x).add(x)).add(B);
        if(!y.sqrt(y2)) { y.zero(); return false; }
        if(y.hasbit(0) != ybit) y.neg();
        return true;
    }
   
    void print(vector<char> &buf)
    {
        normalize();
        buf.add(y.hasbit(0) ? '-' : '+');
        x.printdigits(buf);
    }

    void parse(const char *s)
    {
        bool ybit = *s++ == '-';
        x.parse(s);
        calcy(ybit);
        z = bigint<1>(1);
    }
};

