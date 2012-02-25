--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = off;
SET check_function_bodies = false;
SET client_min_messages = warning;
SET escape_string_warning = off;

--
-- Name: predict; Type: SCHEMA; Schema: -; Owner: postgres
--

CREATE SCHEMA predict;


ALTER SCHEMA predict OWNER TO postgres;

SET search_path = predict, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: account; Type: TABLE; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE TABLE account (
    username text NOT NULL,
    salt text,
    password text,
    added timestamp without time zone
);


ALTER TABLE predict.account OWNER TO postgres;

--
-- Name: attempts; Type: TABLE; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE TABLE attempts (
    file_id text,
    model text,
    calculated timestamp without time zone,
    response text,
    transform text,
    measure_name text,
    response_transform text
);


ALTER TABLE predict.attempts OWNER TO postgres;

--
-- Name: claim; Type: TABLE; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE TABLE claim (
    file text,
    username text,
    added timestamp without time zone
);


ALTER TABLE predict.claim OWNER TO postgres;

--
-- Name: file; Type: TABLE; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE TABLE file (
    file_id text,
    added timestamp without time zone,
    parse_status integer
);


ALTER TABLE predict.file OWNER TO postgres;

--
-- Name: models; Type: TABLE; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE TABLE models (
    added timestamp without time zone DEFAULT now(),
    name text NOT NULL
);


ALTER TABLE predict.models OWNER TO postgres;

--
-- Name: results; Type: TABLE; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE TABLE results (
    file_id text,
    model text,
    calculated timestamp without time zone,
    response text,
    measure double precision,
    json text,
    transform text,
    measure_name text,
    response_transform text
);


ALTER TABLE predict.results OWNER TO postgres;

--
-- Name: session; Type: TABLE; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE TABLE session (
    cookie text,
    username text,
    started timestamp without time zone
);


ALTER TABLE predict.session OWNER TO postgres;

--
-- Name: transforms; Type: TABLE; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE TABLE transforms (
    added timestamp without time zone DEFAULT now(),
    name text NOT NULL
);


ALTER TABLE predict.transforms OWNER TO postgres;

--
-- Name: models_pkey; Type: CONSTRAINT; Schema: predict; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY models
    ADD CONSTRAINT models_pkey PRIMARY KEY (name);


--
-- Name: transforms_pkey; Type: CONSTRAINT; Schema: predict; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY transforms
    ADD CONSTRAINT transforms_pkey PRIMARY KEY (name);


--
-- Name: account_idx; Type: INDEX; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX account_idx ON account USING btree (username);


--
-- Name: attempts_file_idx; Type: INDEX; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE INDEX attempts_file_idx ON attempts USING btree (file_id);


--
-- Name: attempts_idx; Type: INDEX; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE INDEX attempts_idx ON attempts USING btree (file_id, response, model, transform, response_transform);


--
-- Name: claim_uniq; Type: INDEX; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX claim_uniq ON claim USING btree (file);


--
-- Name: claim_uniq2; Type: INDEX; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX claim_uniq2 ON claim USING btree (username, file);


--
-- Name: file_idx; Type: INDEX; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE UNIQUE INDEX file_idx ON file USING btree (file_id);


--
-- Name: results_idx; Type: INDEX; Schema: predict; Owner: postgres; Tablespace: 
--

CREATE INDEX results_idx ON results USING btree (file_id, response, model, transform, response_transform);


--
-- Name: claim_username_fkey; Type: FK CONSTRAINT; Schema: predict; Owner: postgres
--

ALTER TABLE ONLY claim
    ADD CONSTRAINT claim_username_fkey FOREIGN KEY (username) REFERENCES account(username);


--
-- PostgreSQL database dump complete
--

