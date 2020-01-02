--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET lock_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

--
-- Name: plperl; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS plperl WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plperl; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION plperl IS 'PL/Perl procedural language';


--
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


--
-- Name: cube; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS cube WITH SCHEMA public;


--
-- Name: EXTENSION cube; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION cube IS 'data type for multidimensional cubes';


--
-- Name: earthdistance; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS earthdistance WITH SCHEMA public;


--
-- Name: EXTENSION earthdistance; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION earthdistance IS 'calculate great-circle distances on the surface of the Earth';


SET search_path = public, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: msgs; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE msgs (
    id integer NOT NULL,
    ts timestamp with time zone DEFAULT now() NOT NULL,
    devs_fk bigint NOT NULL,
    wx_temp real,
    wx_rh real,
    wx_baro_a real,
    i_bat real,
    v_bat real,
    v_aux1 real,
    v_aux2 real,
    v_aux3 real,
    ad_ch0_raw smallint,
    ad_ch1_raw smallint,
    ad_ch2_raw smallint,
    ad_ch3_raw smallint,
    ad_ch4_raw smallint,
    ad_ch5_raw smallint,
    ad_ch6_raw smallint,
    ad_ch7_raw smallint,
    mote_ts timestamp with time zone,
    mote_flags smallint,
    mote_ctr_up integer,
    gw_frq real,
    gw_sf character varying(16),
    gnss_lat real,
    gnss_lon real,
    gnss_alt_m real,
    gnss_acc_m real,
    gw_id_ary character varying(32)[],
    gw_lat_ary real[],
    gw_lon_ary real[],
    gw_alt_ary real[],
    gw_dbm_ary real[],
    gw_snr_ary real[],
    gw_cnt smallint,
    gnss_spd real,
    gnss_crs real
);


ALTER TABLE public.msgs OWNER TO postgres;

--
-- Name: TABLE msgs; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE msgs IS 'Messages from the devices (motes)';


--
-- Name: COLUMN msgs.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.id IS 'ID';


--
-- Name: COLUMN msgs.ts; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.ts IS 'Timestamp when created';


--
-- Name: COLUMN msgs.devs_fk; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.devs_fk IS 'Foreign key to devs ID';


--
-- Name: COLUMN msgs.wx_temp; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.wx_temp IS 'Temperature in deg C';


--
-- Name: COLUMN msgs.wx_rh; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.wx_rh IS 'Relative humidity in %';


--
-- Name: COLUMN msgs.wx_baro_a; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.wx_baro_a IS 'Barometric pressure in Pa';


--
-- Name: COLUMN msgs.i_bat; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.i_bat IS 'Battery charge current in amps';


--
-- Name: COLUMN msgs.v_bat; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.v_bat IS 'Battery voltage in volts';


--
-- Name: COLUMN msgs.v_aux1; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.v_aux1 IS 'AUX1 voltage';


--
-- Name: COLUMN msgs.v_aux2; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.v_aux2 IS 'AUX2 voltage';


--
-- Name: COLUMN msgs.v_aux3; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.v_aux3 IS 'AUX3 voltage';


--
-- Name: COLUMN msgs.ad_ch0_raw; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.ad_ch0_raw IS 'Raw value of ADC channel 0';


--
-- Name: COLUMN msgs.ad_ch1_raw; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.ad_ch1_raw IS 'Raw value of ADC channel 1';


--
-- Name: COLUMN msgs.ad_ch2_raw; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.ad_ch2_raw IS 'Raw value of ADC channel 2';


--
-- Name: COLUMN msgs.ad_ch3_raw; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.ad_ch3_raw IS 'Raw value of ADC channel 3';


--
-- Name: COLUMN msgs.ad_ch4_raw; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.ad_ch4_raw IS 'Raw value of ADC channel 4';


--
-- Name: COLUMN msgs.ad_ch5_raw; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.ad_ch5_raw IS 'Raw value of ADC channel 5';


--
-- Name: COLUMN msgs.ad_ch6_raw; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.ad_ch6_raw IS 'Raw value of ADC channel 6';


--
-- Name: COLUMN msgs.ad_ch7_raw; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.ad_ch7_raw IS 'Raw value of ADC channel 7';


--
-- Name: COLUMN msgs.mote_ts; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.mote_ts IS 'Timestamp of the mutes clock';


--
-- Name: COLUMN msgs.mote_flags; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.mote_flags IS 'Bitmask of the transfered data';


--
-- Name: COLUMN msgs.mote_ctr_up; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.mote_ctr_up IS 'Counter upstream';


--
-- Name: COLUMN msgs.gw_frq; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gw_frq IS 'Frequency on which message was sent';


--
-- Name: COLUMN msgs.gw_sf; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gw_sf IS 'Data rate used for that message';


--
-- Name: COLUMN msgs.gnss_lat; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gnss_lat IS 'Position of the mote - Latitude';


--
-- Name: COLUMN msgs.gnss_lon; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gnss_lon IS 'Position of the mote - Longitude';


--
-- Name: COLUMN msgs.gnss_alt_m; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gnss_alt_m IS 'Position of the mote - Altitude in meters';


--
-- Name: COLUMN msgs.gnss_acc_m; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gnss_acc_m IS 'Position of the mote - Accuracy in meters';


--
-- Name: COLUMN msgs.gw_id_ary; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gw_id_ary IS 'ID of the gateway catched this message';


--
-- Name: COLUMN msgs.gw_lat_ary; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gw_lat_ary IS 'Position of the gateway - Latitude';


--
-- Name: COLUMN msgs.gw_lon_ary; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gw_lon_ary IS 'Position of the gateway - Longitude';


--
-- Name: COLUMN msgs.gw_alt_ary; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gw_alt_ary IS 'Position of the gateway - Height in meters';


--
-- Name: COLUMN msgs.gw_dbm_ary; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gw_dbm_ary IS 'Signal strength at the gateway in dBm';


--
-- Name: COLUMN msgs.gw_snr_ary; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gw_snr_ary IS 'SNR value at the gateway';


--
-- Name: COLUMN msgs.gw_cnt; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gw_cnt IS 'Count of gateway having heard this message';


--
-- Name: COLUMN msgs.gnss_spd; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gnss_spd IS 'Position of the mote - Speed in km/h';


--
-- Name: COLUMN msgs.gnss_crs; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN msgs.gnss_crs IS 'Position of the mote - Course in degrees';


--
-- Name: Bees; Type: VIEW; Schema: public; Owner: espero
--

CREATE VIEW "Bees" AS
 SELECT msgs.id,
    msgs.ts,
    msgs.devs_fk,
    msgs.mote_ctr_up,
    msgs.ad_ch0_raw,
    msgs.ad_ch1_raw,
    msgs.ad_ch2_raw,
    msgs.ad_ch3_raw,
    msgs.ad_ch4_raw,
    msgs.ad_ch5_raw,
    msgs.ad_ch6_raw,
    msgs.ad_ch7_raw,
    msgs.i_bat,
    msgs.v_bat,
    msgs.v_aux1,
    msgs.v_aux2,
    msgs.v_aux3
   FROM msgs
  ORDER BY msgs.id DESC;


ALTER TABLE public."Bees" OWNER TO espero;

--
-- Name: VIEW "Bees"; Type: COMMENT; Schema: public; Owner: espero
--

COMMENT ON VIEW "Bees" IS 'SELECT id, ts, devs_fk, mote_ctr_up, ad_ch0_raw, ad_ch1_raw, ad_ch2_raw, ad_ch3_raw, ad_ch4_raw, ad_ch5_raw, ad_ch6_raw, ad_ch7_raw, i_bat, v_bat, v_aux1, v_aux2, v_aux3  FROM msgs  ORDER BY id DESC';


--
-- Name: Gateways; Type: VIEW; Schema: public; Owner: espero
--

CREATE VIEW "Gateways" AS
 SELECT msgs.id,
    msgs.ts,
    msgs.devs_fk,
    msgs.mote_ctr_up,
    msgs.gw_sf,
    msgs.gw_frq,
    msgs.gw_cnt,
    msgs.gw_id_ary,
    msgs.gw_dbm_ary,
    msgs.gw_snr_ary,
    msgs.gw_lat_ary,
    msgs.gw_lon_ary,
    msgs.gw_alt_ary
   FROM msgs
  ORDER BY msgs.id DESC;


ALTER TABLE public."Gateways" OWNER TO espero;

--
-- Name: VIEW "Gateways"; Type: COMMENT; Schema: public; Owner: espero
--

COMMENT ON VIEW "Gateways" IS 'SELECT id, ts, devs_fk, mote_ctr_up, gw_sf, gw_frq, gw_cnt, gw_id_ary, gw_dbm_ary, gw_snr_ary, gw_lat_ary, gw_lon_ary, gw_alt_ary  FROM msgs  ORDER BY id DESC';


--
-- Name: Tracking; Type: VIEW; Schema: public; Owner: espero
--

CREATE VIEW "Tracking" AS
 SELECT msgs.id,
    msgs.ts,
    msgs.devs_fk,
    msgs.mote_ctr_up,
    msgs.gnss_lat,
    msgs.gnss_lon,
    msgs.gnss_alt_m,
    msgs.gnss_spd,
    msgs.gnss_crs,
    msgs.gnss_acc_m
   FROM msgs
  ORDER BY msgs.id DESC;


ALTER TABLE public."Tracking" OWNER TO espero;

--
-- Name: VIEW "Tracking"; Type: COMMENT; Schema: public; Owner: espero
--

COMMENT ON VIEW "Tracking" IS 'SELECT id, ts, devs_fk, mote_ctr_up, gnss_lat, gnss_lon, gnss_alt_m, gnss_spd, gnss_crs, gnss_acc_m  FROM msgs  ORDER BY id DESC';


--
-- Name: Weather; Type: VIEW; Schema: public; Owner: espero
--

CREATE VIEW "Weather" AS
 SELECT msgs.id,
    msgs.ts,
    msgs.devs_fk,
    msgs.mote_ctr_up,
    msgs.wx_temp,
    msgs.wx_rh,
    msgs.wx_baro_a
   FROM msgs
  ORDER BY msgs.id DESC;


ALTER TABLE public."Weather" OWNER TO espero;

--
-- Name: VIEW "Weather"; Type: COMMENT; Schema: public; Owner: espero
--

COMMENT ON VIEW "Weather" IS 'SELECT id, ts, devs_fk, mote_ctr_up, wx_temp, wx_rh, wx_baro_a  FROM msgs  ORDER BY id DESC';


--
-- Name: devs; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE devs (
    id integer NOT NULL,
    ts_first timestamp with time zone DEFAULT now() NOT NULL,
    ts_last timestamp with time zone DEFAULT now() NOT NULL,
    dev_id character varying(32) NOT NULL,
    app_id character varying(32) NOT NULL,
    description character varying(256),
    act_method character(1),
    dev_eui character(16) NOT NULL
);


ALTER TABLE public.devs OWNER TO postgres;

--
-- Name: TABLE devs; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON TABLE devs IS 'Devices (motes)';


--
-- Name: COLUMN devs.id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN devs.id IS 'ID';


--
-- Name: COLUMN devs.ts_first; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN devs.ts_first IS 'Time of creation';


--
-- Name: COLUMN devs.ts_last; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN devs.ts_last IS 'Time of last modification/access';


--
-- Name: COLUMN devs.dev_id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN devs.dev_id IS 'Device ID';


--
-- Name: COLUMN devs.app_id; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN devs.app_id IS 'ID of the TTN application';


--
-- Name: COLUMN devs.description; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN devs.description IS 'Description of the device';


--
-- Name: COLUMN devs.act_method; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN devs.act_method IS 'Activation methods: A(BP), O(TAA)';


--
-- Name: COLUMN devs.dev_eui; Type: COMMENT; Schema: public; Owner: postgres
--

COMMENT ON COLUMN devs.dev_eui IS 'Device HW serial ID';


--
-- Name: devs_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE devs_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.devs_id_seq OWNER TO postgres;

--
-- Name: devs_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE devs_id_seq OWNED BY devs.id;


--
-- Name: msgs_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE msgs_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.msgs_id_seq OWNER TO postgres;

--
-- Name: msgs_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE msgs_id_seq OWNED BY msgs.id;


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY devs ALTER COLUMN id SET DEFAULT nextval('devs_id_seq'::regclass);


--
-- Name: id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY msgs ALTER COLUMN id SET DEFAULT nextval('msgs_id_seq'::regclass);


--
-- Name: devs_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY devs
    ADD CONSTRAINT devs_pkey PRIMARY KEY (id);

ALTER TABLE devs CLUSTER ON devs_pkey;


--
-- Name: trks_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY msgs
    ADD CONSTRAINT trks_pkey PRIMARY KEY (id);

ALTER TABLE msgs CLUSTER ON trks_pkey;


--
-- Name: devs_dev_eui_idx; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE INDEX devs_dev_eui_idx ON devs USING btree (dev_eui);


--
-- Name: devsts_idx; Type: INDEX; Schema: public; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX devsts_idx ON msgs USING btree (devs_fk, ts);


--
-- Name: devs_id_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY msgs
    ADD CONSTRAINT devs_id_fk FOREIGN KEY (devs_fk) REFERENCES devs(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- Name: msgs; Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON TABLE msgs FROM PUBLIC;
REVOKE ALL ON TABLE msgs FROM postgres;
GRANT ALL ON TABLE msgs TO postgres;
GRANT SELECT,INSERT,UPDATE ON TABLE msgs TO bees_push;


--
-- Name: devs; Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON TABLE devs FROM PUBLIC;
REVOKE ALL ON TABLE devs FROM postgres;
GRANT ALL ON TABLE devs TO postgres;
GRANT SELECT,INSERT,UPDATE ON TABLE devs TO bees_push;


--
-- Name: devs_id_seq; Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON SEQUENCE devs_id_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE devs_id_seq FROM postgres;
GRANT ALL ON SEQUENCE devs_id_seq TO postgres;
GRANT SELECT,UPDATE ON SEQUENCE devs_id_seq TO bees_push;


--
-- Name: msgs_id_seq; Type: ACL; Schema: public; Owner: postgres
--

REVOKE ALL ON SEQUENCE msgs_id_seq FROM PUBLIC;
REVOKE ALL ON SEQUENCE msgs_id_seq FROM postgres;
GRANT ALL ON SEQUENCE msgs_id_seq TO postgres;
GRANT SELECT,UPDATE ON SEQUENCE msgs_id_seq TO bees_push;


--
-- PostgreSQL database dump complete
--

